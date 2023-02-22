#include "FrameDisplay.h"

#include <3escore/CollatedPacketDecoder.h>
#include <3escore/Endian.h>
#include <3escore/Messages.h>
#include <3escore/PacketBuffer.h>
#include <3escore/PacketWriter.h>
#include <3escore/StreamUtil.h>
#include <3escore/TcpSocket.h>

#include <chrono>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// Note: this program is provided as a C++ implementation of command line packet recording. It is
// equivalent to the 3esrec program which is part of the C#/dotnet 3es project. It is recommended
// that the C# version be use as there are not significant performance differences when running with
// the '-m-' option (passthrough) and the C# code is more fully featured.

#define PACKET_TIMING 0

namespace tes
{
enum class Mode : int
{
  CollateAndCompress,
  CollateOnly,
  FileCompression,
  Uncompressed,
  Passthrough,

  Default = Passthrough
};

class TesRec
{
#if PACKET_TIMING
  static const unsigned PacketLimit = 500u;
#endif  // PACKET_TIMING
public:
  bool quit() const { return _quit; }
  bool argsOk() const { return _argsOk; }
  bool showUsage() const { return _showUsage; }
  bool connected() const { return _connected; }
  bool persist() const { return _persist; }
  bool overwrite() const { return _overwrite; }
  bool quiet() const { return _quiet; }

  Mode decodeMode() const { return _decodeMode; }

  unsigned totalFrames() const { return _totalFrames; }
  // IPEndPoint ServerEndPoint { get; private set; }
  const std::string &outputPrefix() const { return _outputPrefix; }
  static const char *defaultPrefix() { return "tes"; }
  static uint16_t defaultPort() { return 33500; }
  static const char *defaultIP() { return "127.0.0.1"; }

  static const char **defaultArgs() { return s_defaultArgs; }

  static const char **modeArgStrings() { return s_modeArgStrings; }

  static const char *modeToArg(Mode m);

  static Mode argToMode(const char *arg);

  TesRec(int argc, const char **args);

  void usage() const;

  void run(FrameDisplay *frameDisplay);

  void requestQuit() { _quit = true; }

private:
  std::unique_ptr<TcpSocket> attemptConnection();

  std::unique_ptr<std::iostream> createOutputWriter();

  std::string generateNewOutputFile();

  void parseArgs(int argc, const char **argv);

private:
  ServerInfoMessage _serverInfo;
  int _nextOutputNumber = 0;
  unsigned _totalFrames = 0;
  Mode _decodeMode = Mode::Default;
  std::string _outputPrefix = "tes";

  std::string _serverIp;
  uint16_t _serverPort = 0;

  bool _quit = false;
  bool _argsOk = true;
  bool _showUsage = false;
  bool _connected = false;
  bool _persist = false;
  bool _overwrite = false;
  bool _quiet = false;

  static const char *s_defaultArgs[];
  static const size_t s_defaultArgsCount;
  static const char *s_modeArgStrings[];
  static const size_t s_modeArgStringsCount;
};

const char *TesRec::s_defaultArgs[] = { "--ip", "127.0.0.1", "--port", "33500" };

const size_t TesRec::s_defaultArgsCount = sizeof(s_defaultArgs) / sizeof(s_defaultArgs[0]);

const char *TesRec::s_modeArgStrings[] = { "mc", "mC", "mz", "mu", "m-" };

const size_t TesRec::s_modeArgStringsCount = sizeof(s_modeArgStrings) / sizeof(s_modeArgStrings[0]);


const char *TesRec::modeToArg(Mode m)
{
  const int mi = (int)m;
  if (0 <= mi && mi <= (int)s_modeArgStringsCount)
  {
    return s_modeArgStrings[mi];
  }

  return "";
}

Mode TesRec::argToMode(const char *arg)
{
  std::string argStr(arg);
  while (!argStr.empty() && argStr[0] == '-')
  {
    argStr.erase(0, 1);
  }

  for (int i = 0; i < (int)s_modeArgStringsCount; ++i)
  {
    if (argStr.compare(s_modeArgStrings[i]) == 0)
    {
      return (Mode)i;
    }
  }

  return Mode::Default;
}

TesRec::TesRec(int argc, const char **args)
{
  initDefaultServerInfo(&_serverInfo);
  if (argc)
  {
    parseArgs(argc, args);
  }
  else
  {
    parseArgs(int(s_defaultArgsCount), s_defaultArgs);
  }
}


void TesRec::usage() const
{
  printf("Usage:\n"
         "3esrec --ip <server-ip> [--port <server-port>] [prefix]\n"
         "\n"
         "This program attempts to connect to and record a Third Eye Scene server.\n"
         "--help, -?:\n"
         "Show usage.\n"
         "\n"
         "--ip <server-ip>:\n"
         "Specifies the server IP address to connect to.\n"
         "\n"
         // "-m[c,C,v,z,-]:\n"
         // "Specifies how incoming packets are handled. In all modes except m-, incoming\n"
         // "collated packets are first decoded.\n"
         // "- mc : Packet collation and compression. Recollate and compress.\n"
         // "- mC : Packet collation no compression. Recollate only.\n"
         // "- mu : Uncompressed. Save packets as is. No compression.\n"
         // "- mz : File level compression. Decode incoming packets and compress at the\n"
         // "        file level.\n"
         // "- m- : Passthrough. Packets are saved exactly as they come in.\n"
         // "The fastest mode is -m- as this performs no additional calculations other than\n"
         // "CRC validation. However, this mode requires naked frame packets for accurate\n"
         // "frame count finalisation.\n"
         // "\n"
         // "The default mode is: %s\n"
         // "\n"
         "--port <server-port>:\n"
         "Specifies the port to connect on.  The default port is %d\n"
         "\n"
         "--persist, -p:\n"
         "Persist beyond the first connection. The program keeps running awaiting\n"
         "further connections. Use Control-C to terminate.\n"
         "\n"
         "--quiet, -q:\n"
         "Run in quiet mode (disable non-critical logging).\n"
         "\n"
         "--overwrite, -w:\n"
         "Overwrite existing files using the current prefix. The current session\n"
         "numbering will not overwrite until they loop to 0.\n"
         "\n"
         "[prefix]:\n"
         "Specifies the file prefix used for recording. The recording file is\n"
         "formulated as {prefix###.3es}, where the number used is the first missing\n"
         "file up to 999. At that point the program will complain that there are no\n"
         "more available file names.\n",
         defaultPort());
  // , modeToArg(Mode::Default), defaultPort());
}

void TesRec::run(FrameDisplay *frameDisplay)
{
  int connectionPollTimeSecMs = 250;
  std::vector<uint8_t> socketBuffer(4 * 1024);
  std::vector<uint8_t> decodeBuffer(4 * 1024);
  std::unique_ptr<TcpSocket> socket = nullptr;
  std::unique_ptr<PacketBuffer> packetBuffer;
  std::unique_ptr<std::iostream> ioStream;
  CollatedPacketDecoder collatedDecoder;
#if PACKET_TIMING
  using TimingClock = std::chrono::high_resolution_clock;
  auto startTime = TimingClock::now();                  // Set startTime type
  auto timingElapsed = TimingClock::now() - startTime;  // Set timingElapsed type
#endif                                                  // PACKET_TIMING
  bool once = true;

  if (!_quiet)
  {
    printf("Connecting to %s:%d\n", _serverIp.c_str(), _serverPort);
  }

  while (!_quit && (_persist || once))
  {
    once = false;
    // First try establish a connection.
    while (!_quit && !_connected)
    {
      socket = std::move(attemptConnection());
      if (socket)
      {
#if PACKET_TIMING
        startTime = TimingClock::now();
#endif  // PACKET_TIMING
        _totalFrames = 0u;
        frameDisplay->reset();
        if (!_quiet)
        {
          frameDisplay->start();
        }

        ioStream = std::move(createOutputWriter());
        if (ioStream)
        {
          _connected = true;
          // Create a new packet buffer for this connection.
          packetBuffer = std::make_unique<PacketBuffer>();
        }
        // Log.Flush();
      }
      else
      {
        // Log.Flush();
        // Wait the timeout period before attempting to reconnect.
        std::this_thread::sleep_for(std::chrono::milliseconds(connectionPollTimeSecMs));
      }
    }

    // Read while connected or data still available.
    bool haveData = false;
    while (!_quit && socket && (socket->isConnected() || haveData))
    {
      // We have a connection. Read messages while we can.
      int bytesRead = socket->readAvailable(socketBuffer.data(), int(socketBuffer.size()));
      haveData = false;
      if (bytesRead <= 0)
      {
        std::this_thread::sleep_for(std::chrono::microseconds(500));
        continue;
      }

      haveData = true;
      packetBuffer->addBytes(socketBuffer.data(), bytesRead);

      while (PacketHeader *newPacketHeader = packetBuffer->extractPacket(decodeBuffer))
      {
        PacketReader completedPacket(newPacketHeader);

        if (!completedPacket.checkCrc())
        {
          printf("CRC failure\n");
          continue;
        }

        // TODO: Check for dropped byte in the packet buffer.

        if (_decodeMode == Mode::Passthrough)
        {
          ioStream->write((const char *)newPacketHeader, completedPacket.packetSize());

          if (completedPacket.routingId() == MtControl)
          {
            if (completedPacket.messageId() == CIdFrame)
            {
              ++_totalFrames;
              frameDisplay->incrementFrame();
#if PACKET_TIMING
              if (_totalFrames >= PacketLimit)
              {
                timingElapsed = TimingClock::now() - startTime;
                _quit = true;
              }
#endif  // PACKET_TIMING
            }
          }
        }
        else
        {
          // Decode and decompress collated packets. This will just return the same packet
          // if not collated.
          collatedDecoder.setPacket(newPacketHeader);
          while (const PacketHeader *decodedPacketHeader = collatedDecoder.next())
          {
            PacketReader decodedPacket(decodedPacketHeader);
            bool exportPacket = true;

            // Console.WriteLine("Msg: {0} {1}", completedPacket.Header.RoutingID,
            // completedPacket.Header.MessageID);
            switch (completedPacket.routingId())
            {
            case MtControl:
              if (decodedPacket.messageId() == CIdFrame)
              {
                ++_totalFrames;
                frameDisplay->incrementFrame();
#if PACKET_TIMING
                if (_totalFrames >= PacketLimit)
                {
                  timingElapsed = TimingClock::now() - startTime;
                  _quit = true;
                }
#endif  // PACKET_TIMING
              }
              break;

            case MtServerInfo:
              if (_serverInfo.read(decodedPacket))
              {
                printf("Failed to decode ServerInfo message\n");
                _quit = true;
              }
              break;

            default:
              break;
            }

            if (exportPacket)
            {
              // TODO: use a local collated packet to re-compress data.
              ioStream->write((const char *)newPacketHeader, completedPacket.packetSize());
            }
          }
        }
      }
    }

    frameDisplay->stop();

    if (ioStream)
    {
      streamutil::finaliseStream(*ioStream, _totalFrames);
      ioStream->flush();
      ioStream.reset(nullptr);
    }

    if (!_quiet)
    {
      printf("\nConnection closed\n");
    }

    // Disconnected.
    if (socket)
    {
      socket->close();
      socket.reset(nullptr);
    }

    _connected = false;
  }

#if PACKET_TIMING
  const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(timingElapsed);
  printf("Processed %d packets in %dms\n", PacketLimit, int(elapsedMs.count()));
#endif  // PACKET_TIMING
}

std::unique_ptr<TcpSocket> TesRec::attemptConnection()
{
  std::unique_ptr<TcpSocket> socket(new TcpSocket);

  if (socket->open(_serverIp.c_str(), _serverPort))
  {
    socket->setNoDelay(true);
    socket->setWriteTimeout(0);
    socket->setReadTimeout(0);
    return std::move(socket);
  }

  return nullptr;
}


std::unique_ptr<std::iostream> TesRec::createOutputWriter()
{
  std::string filePath = generateNewOutputFile();
  if (filePath.empty())
  {
    printf("Unable to generate a numbered file name using the prefix: %s. Try cleaning up the "
           "output directory.\n",
           _outputPrefix.c_str());
    return nullptr;
  }
  printf("Recording to: %s\n", filePath.c_str());

  std::unique_ptr<std::fstream> stream(new std::fstream);

  stream->open(filePath.c_str(), std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc);

  if (!stream->is_open())
  {
    return nullptr;
  }

  // Write the recording header uncompressed to the file.
  // We'll rewind here later and update the frame count.
  // Write to a memory stream to prevent corruption of the file stream when we wrap it
  // in a GZipStream.
  streamutil::initialiseStream(*stream, &_serverInfo);

  return std::move(stream);

  // TODO: implement compression modes
  // switch (DecodeMode)
  // {
  //   case Mode.CollateAndCompress:
  //     stream = new CollationStream(fileStream, true);
  //     break;
  //   case Mode.CollateOnly:
  //     stream = new CollationStream(fileStream, false);
  //     break;
  //   case Mode.FileCompression:
  //     stream = new GZipStream(fileStream, CompressionMode.Compress,
  //     CompressionLevel.BestCompression); break;
  //   case Mode.Uncompressed:
  //     stream = fileStream;
  //     break;
  // }

  // return new NetworkWriter(stream);
}


std::string TesRec::generateNewOutputFile()
{
  const int maxFiles = 1000;
  std::string outputPath;
  _nextOutputNumber = _nextOutputNumber % maxFiles;
  for (int i = _nextOutputNumber; i < maxFiles; ++i)
  {
    std::ostringstream outputPathStream;
    outputPathStream << _outputPrefix << std::setw(3) << std::setfill('0') << i << ".3es";
    outputPath = outputPathStream.str();

    // Check if the file exists.
    bool pathOk = _overwrite;
    if (!pathOk)
    {
      std::ifstream inTest(outputPath.c_str());
      pathOk = !inTest.is_open();
    }

    if (pathOk)
    {
      _nextOutputNumber = i + 1;
      return outputPath;
    }
  }

  return std::string();
}


void TesRec::parseArgs(int argc, const char **argv)
{
  bool ok = argc > 0;
  std::string ipStr;
  std::string portStr;
  bool output_prefix_set = false;

  _argsOk = false;
  for (int i = 1; i < argc; ++i)
  {
    std::string arg(argv[i]);
    if (arg.compare("--help") == 0 || arg.compare("-?") == 0 || arg.compare("-h") == 0)
    {
      _showUsage = true;
    }
    else if (arg.compare("--ip") == 0)
    {
      if (i + 1 < argc)
      {
        ipStr = argv[++i];
      }
      else
      {
        ok = false;
      }
    }
    else if (arg.find("-m") == 0)
    {
      _decodeMode = argToMode(arg.c_str());
    }
    else if (arg.compare("--overwrite") == 0 || arg.compare("-w") == 0)
    {
      _overwrite = true;
    }
    else if (arg.compare("--persist") == 0 || arg.compare("-w") == 0)
    {
      _persist = true;
    }
    else if (arg.compare("--quiet") == 0 || arg.compare("-q") == 0)
    {
      _quiet = true;
      // printf("Setting Quiet\n");
    }
    else if (arg.compare("--port") == 0)
    {
      if (i + 1 < argc)
      {
        std::string portStr = argv[++i];
        std::istringstream portIn(portStr);
        portIn >> _serverPort;
        if (portIn.bad())
        {
          printf("Error parsing port\n");
          ok = false;
        }
      }
      else
      {
        ok = false;
      }
    }
    else if (!output_prefix_set && arg.find("-") != 0)
    {
      _outputPrefix = arg;
      output_prefix_set = true;
    }
  }

  if (ok)
  {
    if (_serverPort == 0)
    {
      _serverPort = defaultPort();
    }

    if (ipStr.empty())
    {
      ipStr = defaultIP();
    }

    if (!ipStr.empty() && _serverPort > 0)
    {
      _serverIp = ipStr;
    }
    else
    {
      ok = false;
      printf("Missing valid server IP address and port.\n");
    }
  }

  if (_outputPrefix.empty())
  {
    _outputPrefix = defaultPrefix();
  }

  _argsOk = ok;
}
}  // namespace tes

namespace
{
tes::TesRec *g_prog = nullptr;

void onSignal(int)
{
  if (g_prog)
  {
    g_prog->requestQuit();
    g_prog = nullptr;
  }
}
}  // namespace

int main(int argc, const char **argv)
{
  tes::TesRec prog(argc, argv);
  g_prog = &prog;

  signal(SIGINT, onSignal);
  signal(SIGTERM, onSignal);

  if (prog.showUsage() || !prog.argsOk())
  {
    prog.usage();
    return 1;
  }

  tes::FrameDisplay frameDisplay;
  prog.run(&frameDisplay);
  frameDisplay.stop();

  g_prog = nullptr;

  return 0;
}
