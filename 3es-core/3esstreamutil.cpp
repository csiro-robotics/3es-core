//
// author: Kazys Stepanas
//
#include "3esstreamutil.h"

#include "3esmessages.h"
#include "3espacketwriter.h"

#include <iostream>
#include <vector>

namespace tes
{
  namespace streamutil
  {
    bool initialiseStream(std::ostream &stream, const ServerInfoMessage *serverInfo)
    {
      const uint16_t packetBufferSize = 256;
      uint8_t packetBuffer[packetBufferSize];
      PacketWriter packet(packetBuffer, packetBufferSize);

      if (serverInfo)
      {
        packet.reset(MtServerInfo, 0);
        if (!serverInfo->write(packet))
        {
          return false;
        }

        if (!packet.finalise())
        {
          return false;
        }

        stream.write(reinterpret_cast<const char *>(packet.data()), packet.packetSize());
        if (!stream.good() || stream.fail())
        {
          return false;
        }
      }

      // Write a frame count control message place holder.
      packet.reset(MtControl, CIdFrameCount);
      ControlMessage msg;
      msg.controlFlags = 0;
      msg.value32 = 0;
      msg.value64 = 0;

      if (!msg.write(packet))
      {
        return false;
      }

      if (!packet.finalise())
      {
        return false;
      }

      stream.write(reinterpret_cast<const char *>(packet.data()), packet.packetSize());
      if (!stream.good() || stream.fail())
      {
        return false;
      }

      return true;
    }


    bool finaliseStream(std::iostream &stream, unsigned frameCount)
    {
      // Rewind the stream to the beginning and find the first RoutingID.ServerInfo message
      // and RoutingID.Control message with a ControlMessageID.FrameCount ID. These should be
      // the first and second messages in the stream.
      // We'll limit searching to the first 5 messages.
      std::streampos serverInfoMessageStart = -1;
      std::streampos frameCountMessageStart = -1;
      stream.flush();

      // Record the initial stream position to restore later.
      std::streampos restorePos = stream.tellp();

      // Set the read position to search for the relevant messages.
      stream.seekg(0);

      std::streampos streamPos = 0;

      std::vector<uint8_t> headerBuffer(1024);
      auto markerValidation = PacketMarker;
      const char *markerValidationBytes = (const char *)&markerValidation;
      networkEndianSwap(markerValidation);
      decltype(PacketMarker) marker = 0;
      char *markerBytes = (char *)&marker;
      bool markerValid = false;

      int attemptsRemaining = 5;
      int byteReadLimit = 0;

      while ((frameCountMessageStart < 0 || serverInfoMessageStart < 0) && attemptsRemaining > 0 && stream.good())
      {
        --attemptsRemaining;
        markerValid = false;

        // Limit the number of bytes we try read in each attempt.
        byteReadLimit = 1024;
        while (byteReadLimit > 0)
        {
          --byteReadLimit;
          // Record the potential marker start position.
          streamPos = stream.tellg();
          stream.read(markerBytes, 1);
          if (markerBytes[0] == markerValidationBytes[0])
          {
            markerValid = true;
            int i = 1;
            for (i = 1; markerValid && stream.good() && i < int(sizeof(markerValidation)); ++i)
            {
              stream.read(markerBytes + i, 1);
              markerValid = markerValid && markerBytes[i] == markerValidationBytes[i];
            }

            if (markerValid)
            {
              break;
            }
            else
            {
              // We've failed to fully validate the maker. However, we did read and validate
              // one byte in the marker, then continued reading until the failure. It's possible
              // that the last byte read, the failed byte, may be the start of the actual marker.
              // We check this below, and if so, we rewind the stream one byte in order to
              // start validation from there on the next iteration. We can ignore the byte if
              // it is does not match the first validation byte. We are unlikely to ever make this
              // match though.
              --i;  // Go back to the last read byte.
              if (markerBytes[i] == markerValidationBytes[0])
              {
                // Potentially the start of a new marker. Rewind the stream to attempt to validate it.
                stream.seekg(-1, std::ios_base::cur);
              }
            }
          }
        }

        if (markerValid && stream.good())
        {
          // Potential packet target. Return to the start of the marker.
          stream.seekg(streamPos);

          // Read the packet header.
          stream.read((char *)headerBuffer.data(), sizeof(PacketHeader));
          auto bytesRead = stream.tellg() - streamPos;
          if (bytesRead == sizeof(PacketHeader))
          {
            // Resolve the packet size.
            unsigned packetSize = PacketReader((PacketHeader *)headerBuffer.data()).packetSize();

            // Read additional bytes.
            if (packetSize > sizeof(PacketHeader))
            {
              const auto preReadPos = stream.tellg();
              stream.read((char *)headerBuffer.data() + sizeof(PacketHeader), packetSize - sizeof(PacketHeader));
              bytesRead = stream.tellg() - preReadPos;
            }

            // Convert into a PacketReader to support endian fixes.
            const PacketReader currentPacket((const PacketHeader *)headerBuffer.data());

            // Check packet/message type.
            if (currentPacket.routingId() == MtServerInfo)
            {
              serverInfoMessageStart = streamPos;
            }
            else if (currentPacket.routingId() == MtControl && currentPacket.messageId() == CIdFrameCount)
            {
              // It's the frame count control message. Set the offset to the frame count member.
              frameCountMessageStart = streamPos;
            }
            else
            {
              // At this point, we've failed to find the right kind of header. We could use the payload size to
              // skip ahead in the stream which should align exactly to the next message.
              // Not done for initial testing.
            }
          }
        }
      }

      // if (serverInfoMessageStart >= 0)
      // {
      //   // Found the correct location. Seek the stream to here and write a new FrameCount control message.
      //   stream.seekp(serverInfoMessageStart);
      //   PacketWriter packet(headerBuffer.data(), uint16_t(headerBuffer.size()), MtServerInfo);

      //   _serverInfo.write(packet);
      //   packet.finalise();
      //   stream.write((const char *)headerBuffer.data(), packet.packetSize());
      //   stream.flush();
      // }

      if (frameCountMessageStart >= 0)
      {
        // Found the correct location. Seek the stream to here and write a new FrameCount control message.
        stream.seekp(frameCountMessageStart);

        PacketWriter packet(headerBuffer.data(), uint16_t(headerBuffer.size()), MtControl, CIdFrameCount);
        ControlMessage frameCountMsg;

        frameCountMsg.controlFlags = 0;
        frameCountMsg.value32 = frameCount;
        frameCountMsg.value64 = 0;

        frameCountMsg.write(packet);
        packet.finalise();
        stream.write((const char *)headerBuffer.data(), packet.packetSize());
        stream.flush();
      }

      if (restorePos > 0)
      {
        stream.seekp(restorePos);
        stream.seekg(restorePos);
        stream.flush();
      }

      return stream.good();
    }
  } // namespace streamutil
} // namespace tes