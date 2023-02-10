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

namespace tes
{
MeshSet::MeshSet(const Id &id, const UIntArg &part_count)
  : Shape(SIdMeshSet, id)
  , _part_count(part_count)
{
  _parts = new Part[part_count.i];
  if (part_count)
  {
    for (unsigned i = 0; i < part_count; ++i)
    {
      _parts[i].transform = Transform::identity();
    }
  }
}


MeshSet::MeshSet(const MeshResource *part, const Id &id)
  : Shape(SIdMeshSet, id)
  , _parts(new Part[1])
  , _part_count(1)
{
  _parts[0].resource = part;
}


MeshSet::MeshSet(const MeshSet &other)
  : Shape(other)
{
  other.onClone(this);
}


MeshSet::MeshSet(MeshSet &&other) noexcept
  : Shape(other)
  , _parts(std::exchange(other._parts, nullptr))
  , _part_count(std::exchange(other._part_count, 0))
  , _own_part_resources(std::exchange(other._own_part_resources, false))
{}

MeshSet::~MeshSet()
{
  cleanupParts();
}


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
  ObjectAttributesd attr;
  uint32_t part_id = 0;
  uint16_t number_of_parts = 0;

  bool ok = true;

  std::memset(&attr, 0, sizeof(attr));

  ok = ok && stream.readElement(number_of_parts) == sizeof(number_of_parts);

  if (ok && number_of_parts > _part_count)
  {
    cleanupParts();

    _parts = new Part[number_of_parts];
    _own_part_resources = true;

    for (unsigned i = 0; i < number_of_parts; ++i)
    {
      _parts[i] = Part();
    }

    _part_count = number_of_parts;
  }

  const bool expect_double_precision = (_data.flags & OFDoublePrecision) != 0;
  for (unsigned i = 0; i < _part_count; ++i)
  {
    ok = ok && stream.readElement(part_id) == sizeof(part_id);
    ok = ok && attr.read(stream, expect_double_precision);

    if (ok)
    {
      _parts[i].transform =
        Transform(Vector3d(attr.position), Quaterniond(attr.rotation), Vector3d(attr.scale));
      _parts[i].transform.setPreferDoublePrecision(expect_double_precision);
      // We can only reference dummy meshes here.
      _parts[i].resource = new MeshPlaceholder(part_id);
      _parts[i].colour = Colour(attr.colour);
    }
  }

  return ok;
}


unsigned MeshSet::enumerateResources(const Resource **resources, unsigned capacity,
                                     unsigned fetch_offset) const
{
  if (!resources || !capacity)
  {
    return _part_count;
  }

  if (fetch_offset >= _part_count)
  {
    return 0;
  }

  const unsigned copy_count = std::min(capacity, _part_count - fetch_offset);
  const Part *parts = _parts + fetch_offset;
  const Resource **dst = resources;
  for (unsigned i = 0; i < copy_count; ++i)
  {
    *dst++ = parts->resource;
    ++parts;
  }
  return copy_count;
}


Shape *MeshSet::clone() const
{
  auto *copy = new MeshSet(_part_count);
  onClone(copy);
  return copy;
}


void MeshSet::onClone(MeshSet *copy) const
{
  Shape::onClone(copy);
  if (copy->_part_count != _part_count)
  {
    delete[] copy->_parts;
    if (_part_count)
    {
      copy->_parts = new Part[_part_count];
    }
    else
    {
      copy->_parts = nullptr;
    }
  }
  copy->_own_part_resources = false;
  std::copy(_parts, _parts + _part_count, copy->_parts);
}


void MeshSet::cleanupParts()
{
  if (_own_part_resources && _parts)
  {
    for (unsigned i = 0; i < _part_count; ++i)
    {
      delete _parts[i].resource;
    }
  }

  delete[] _parts;

  _parts = nullptr;
  _own_part_resources = false;
}
}  // namespace tes
