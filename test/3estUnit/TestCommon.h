//
// author: Kazys Stepanas
//
// Test utility and helper functions.
//

#include <3escore/Colour.h>
#include <3escore/Vector3.h>

#include <cinttypes>
#include <memory>
#include <unordered_map>
#include <vector>

namespace tes
{
class DataBuffer;
class MeshResource;
class MeshSet;
class MeshShape;
class Resource;
class Shape;
class Text2D;
class Text3D;

using ResourceMap = std::unordered_map<uint64_t, std::shared_ptr<Resource>>;

void makeHiResSphere(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
                     std::vector<Vector3f> *normals);
void makeLowResSphere(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
                      std::vector<Vector3f> *normals);
void makeSphere(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
                std::vector<Vector3f> *normals, int iterations);

void validateMesh(const MeshResource &mesh, const MeshResource &reference);
void validateShape(const Shape &shape, const Shape &reference, const ResourceMap &resources);
void validateShape(const Text2D &shape, const Text2D &reference, const ResourceMap &resources);
void validateShape(const Text3D &shape, const Text3D &reference, const ResourceMap &resources);
void validateShape(const MeshShape &shape, const MeshShape &reference,
                   const ResourceMap &resources);
void validateShape(const MeshSet &shape, const MeshSet &reference, const ResourceMap &resources);

/// Read a colour from a @c DataStream.
///
/// Supports reading from single channel @c uint32_t streams or 4 channel @c uint8_t streams.
///
/// @param stream The data stream to read from.
/// @param index The index to read from.
/// @param colour The object to read into.
/// @return True when the stream is of a valid form and index in range. False on failure.
bool getColour(const DataBuffer &stream, size_t index, Colour &colour);
}  // namespace tes
