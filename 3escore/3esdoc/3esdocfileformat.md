
# File format {#docfileformat}

3rd Eye Scene files are client recordings from a 3<sup>rd</sup> Eye Scene server connection. 3rd Eye Scene files normally have a '.3es' extension. The simplest such file is simply a direct
serialisation of the incoming message packets - see [protocol documentation](#docprotocol). The incoming messages are serialised as is; packet header, payload and CRC. However, a well formed 3es file should follow the guidelines described below and may optionally compress the file.

## Layout

A well formed 3es file is always begins with a [server info](#server-info-messages) message. This identifies the server characteristics and coordinate frame for correct playback. This message is immediately followed by a *frame count* [control message](#control-messages). This identifies the total number of recorded frames in the file. Both these messages must always appear in the file uncompressed. The remained of the stream may uncompressed, fully compressed with GZip compression or compressed in a series of `CollatedPacket` sections.

Item                      | Description
------------------------- | ---------------------------------------------------
Server info               | A [server info](#server-info-messages) message detailing the server setting.
Frame count               | Optional [control message](#control-messages) setting the number of frames in the recording.
[Compression begins]      | Optional GZIP compression begins here. Preceding data are uncompressed.
World state serialisation | Optional (recommended) serialisation of the world state at the start of recording.
Message serialisation     | Recorded messages.

Data following the server info and frame count control message may appear either compressed or uncompressed. The two leading message are then followed by a serialisation of the client world state at the start of recording. That is, on beginning recording, the viewer will serialise create and data messages required to instantiate the objects already present on the viewing client. This ensures the correct initial state for subsequent messages.

The remainder of the file is simply a serialisation of the incoming server message packets with optional file level compression.

### File options

The following components of a 3es recording are optional, though some are highly recommended and should generally be expected to be present:

- A frame count [control message](#control-messages) following the server info message (highly recommended).
- World state serialisation as described above (highly recommended).
- [Compression](#compression)
- [Camera messages](#camera-messages) representing the viewing application's camera state.

Note the world state serialisation is optional, but highly recommended. While 3<sup>3r</sup> Eye Scene viewer does serialise the world state, some recording applications may be unable to recognise the world state. For instance, the core release of 3<sup>3r</sup> Eye Scene also features a command line recording application. This application is a dumb application which does nothing to interpret the incoming messages. As such it does not know the world state and cannot serialise one. Such an application is intended to connect as the server application starts and record all incoming packets, thus obviating the need for a world state serialisation.

Camera messages are also optional, primarily because not all recording applications have a client camera representation (such as the command line recording application).

### Compression

3es files use GZIP compression. The recommended supporting libraries for GZIP compression are [zlib](http://www.zlib.net/ in GZIP mode, [GZipStream](https://msdn.microsoft.com/en-us/library/system.io.compression.gzipstream(v=vs.110).aspx from the .Net core libraries, or the `GZipStream` port to Unity provided as part of the Tes.Compression namespace, thanks to [Hitcents](http://www.hitcents.com/).

There are two ways compression can be applied to 3es data streams - whole file compression and [collated packet compression](#collated-packet-messages). With whole file compression, GZIP compression begins immediately after the expected [server info](#server-info-messages) and frame count messages. Collated packets typically collated and compress messages for a single frame update and allow for seeking to frames in the data file and is thus the preferred compression format.
