//
// author: Kazys Stepanas
//
// Test utility and helper functions.
//

#include <3escore/Vector3.h>

#include <cinttypes>
#include <unordered_map>
#include <vector>

namespace tes
{
class MeshResource;
class MeshSet;
class MeshShape;
class PointCloudShape;
class Resource;
class Shape;
class Text2D;
class Text3D;

using ResourceMap = std::unordered_map<uint64_t, Resource *>;

void makeHiResSphere(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> *normals);
void makeLowResSphere(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> *normals);
void makeSphere(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> *normals,
                int iterations);

void validateMesh(const MeshResource &mesh, const MeshResource &reference);
void validateShape(const Shape &shape, const Shape &reference, const ResourceMap &resources);
void validateShape(const Text2D &shape, const Text2D &reference, const ResourceMap &resources);
void validateShape(const Text3D &shape, const Text3D &reference, const ResourceMap &resources);
void validateShape(const MeshShape &shape, const MeshShape &reference, const ResourceMap &resources);
void validateShape(const PointCloudShape &shape, const PointCloudShape &reference, const ResourceMap &resources);
void validateShape(const MeshSet &shape, const MeshSet &reference, const ResourceMap &resources);
}  // namespace tes
