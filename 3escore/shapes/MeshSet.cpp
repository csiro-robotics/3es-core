//
// author: Kazys Stepanas
//
#include "MeshSet.h"

#include "MeshPlaceholder.h"

#include <3escore/CoreUtil.h>
#include <3escore/MeshMessages.h>
#include <3escore/Rotation.h>

#include <algorithm>
#include <cstring>
#include <iterator>

namespace tes
{
MeshSet::MeshSet(const Id &id, const UIntArg &part_count)
  : Shape(SIdMeshSet, id)
{
  _parts.resize(part_count.i);
}


MeshSet::MeshSet(MeshResourcePtr part, const Id &id)
  : Shape(SIdMeshSet, id)
{
  _parts.emplace_back();
  _parts[0].resource = std::move(part);
}


MeshSet::MeshSet(const MeshSet &other)
  : Shape(other)
{
  other.onClone(*this);
}


MeshSet::MeshSet(MeshSet &&other) noexcept = default;


MeshSet::~MeshSet() = default;


bool MeshSet::writeCreate(PacketWriter &stream) const
{
  if (!Shape::writeCreate(stream))
  {
    return false;
  }

  ObjectAttributesd attr;
  uint32_t part_id = 0;
  const auto number_of_parts = int_cast<uint16_t>(partCount());

  std::memset(&attr, 0, sizeof(attr));

  stream.writeElement(number_of_parts);

  bool ok = true;
  for (int i = 0; i < number_of_parts; ++i)
  {
    const Part &part = _parts[i];
    if (part.resource)
    {
      part_id = part.resource->id();
    }
    else
    {
      // Write a dummy.
      part_id = 0;
    }

    attr.position[0] = part.transform.position()[0];
    attr.position[1] = part.transform.position()[1];
    attr.position[2] = part.transform.position()[2];
    attr.rotation[0] = part.transform.rotation()[0];
    attr.rotation[1] = part.transform.rotation()[1];
    attr.rotation[2] = part.transform.rotation()[2];
    attr.rotation[3] = part.transform.rotation()[3];
    attr.scale[0] = part.transform.scale()[0];
    attr.scale[1] = part.transform.scale()[1];
    attr.scale[2] = part.transform.scale()[2];
    attr.colour = part.colour.colour32();

    ok = stream.writeElement(part_id) == sizeof(part_id) && ok;
    // The precision of the transforms is determined by the CreateMessage::flag OFDoublePrecision
    // only.
    if (_data.flags & OFDoublePrecision)
    {
      ok = attr.write(stream) && ok;
    }
    else
    {
      ok = static_cast<ObjectAttributesf>(attr).write(stream) && ok;
    }
  }

  return ok;
}


bool MeshSet::readCreate(PacketReader &stream)
{
  if (!Shape::readCreate(stream))
  {
    return false;
  }

  // Setup attributes to support double precision. Actual read depends on the CreateMessage flag
  // OFDoublePrecision.
  ObjectAttributesd attr = {};
  uint32_t part_id = 0;
  uint16_t number_of_parts = 0;

  bool ok = true;

  ok = ok && stream.readElement(number_of_parts) == sizeof(number_of_parts);
  _parts.resize(number_of_parts);

  const bool expect_double_precision = (_data.flags & OFDoublePrecision) != 0;
  for (unsigned i = 0; i < number_of_parts; ++i)
  {
    ok = ok && stream.readElement(part_id) == sizeof(part_id);
    ok = ok && attr.read(stream, expect_double_precision);

    if (ok)
    {
      _parts[i].transform =
        Transform(Vector3d(attr.position), Quaterniond(attr.rotation), Vector3d(attr.scale));
      _parts[i].transform.setPreferDoublePrecision(expect_double_precision);
      // We can only reference dummy resources here.
      _parts[i].resource = std::make_shared<MeshPlaceholder>(part_id);
      _parts[i].colour = Colour(attr.colour);
    }
  }

  return ok;
}


unsigned MeshSet::enumerateResources(std::vector<ResourcePtr> &resources) const
{
  for (const auto &part : _parts)
  {
    resources.emplace_back(part.resource);
  }
  return int_cast<unsigned>(_parts.size());
}


std::shared_ptr<Shape> MeshSet::clone() const
{
  auto copy = std::make_shared<MeshSet>();
  onClone(*copy);
  return copy;
}


void MeshSet::onClone(MeshSet &copy) const
{
  Shape::onClone(copy);
  copy._parts.clear();
  std::copy(_parts.begin(), _parts.end(), std::back_inserter(copy._parts));
}
}  // namespace tes
