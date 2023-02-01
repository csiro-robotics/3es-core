//
// author: Kazys Stepanas
//
#include "MeshSet.h"

#include "MeshPlaceholder.h"

#include <3escore/MeshMessages.h>
#include <3escore/Rotation.h>

#include <algorithm>
#include <cstring>

using namespace tes;

MeshSet::MeshSet(const Id &id, const UIntArg &partCount)
  : Shape(SIdMeshSet, id)
  // , _parts(partCount.i ? new Part[partCount.i] : nullptr)
  , _parts(nullptr)
  , _partCount(partCount)
  , _ownPartResources(false)
{
  _parts = new Part[partCount.i];
  if (partCount)
  {
    for (unsigned i = 0; i < partCount; ++i)
    {
      _parts[i].transform = Transform::identity();
    }
  }
}


MeshSet::MeshSet(const MeshResource *part, const Id &id)
  : Shape(SIdMeshSet, id)
  , _parts(new Part[1])
  , _partCount(1)
  , _ownPartResources(false)
{
  _parts[0].resource = part;
}


MeshSet::MeshSet(const MeshSet &other)
  : Shape(other)
  , _parts(nullptr)
  , _partCount(0)
  , _ownPartResources(false)
{
  other.onClone(this);
}


MeshSet::MeshSet(MeshSet &&other)
  : Shape(other)
  , _parts(std::exchange(other._parts, nullptr))
  , _partCount(std::exchange(other._partCount, 0))
  , _ownPartResources(std::exchange(other._ownPartResources, false))
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
  uint32_t partId;
  uint16_t numberOfParts = uint16_t(partCount());

  std::memset(&attr, 0, sizeof(attr));

  stream.writeElement(numberOfParts);

  bool ok = true;
  for (int i = 0; i < numberOfParts; ++i)
  {
    const Part &part = _parts[i];
    if (part.resource)
    {
      partId = part.resource->id();
    }
    else
    {
      // Write a dummy.
      partId = 0;
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

    ok = stream.writeElement(partId) == sizeof(partId) && ok;
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
  uint32_t partId = 0;
  uint16_t numberOfParts = 0;

  bool ok = true;

  std::memset(&attr, 0, sizeof(attr));

  ok = ok && stream.readElement(numberOfParts) == sizeof(numberOfParts);

  if (ok && numberOfParts > _partCount)
  {
    cleanupParts();

    _parts = new Part[numberOfParts];
    _ownPartResources = true;

    for (unsigned i = 0; i < numberOfParts; ++i)
    {
      _parts[i] = Part();
    }

    _partCount = numberOfParts;
  }

  const bool expect_double_precision = (_data.flags & OFDoublePrecision) != 0;
  for (unsigned i = 0; i < _partCount; ++i)
  {
    ok = ok && stream.readElement(partId) == sizeof(partId);
    ok = ok && attr.read(stream, expect_double_precision);

    if (ok)
    {
      _parts[i].transform =
        Transform(Vector3d(attr.position), Quaterniond(attr.rotation), Vector3d(attr.scale));
      _parts[i].transform.setPreferDoublePrecision(expect_double_precision);
      // We can only reference dummy meshes here.
      _parts[i].resource = new MeshPlaceholder(partId);
      _parts[i].colour = Colour(attr.colour);
    }
  }

  return ok;
}


unsigned MeshSet::enumerateResources(const Resource **resources, unsigned capacity,
                                     unsigned fetchOffset) const
{
  if (!resources || !capacity)
  {
    return _partCount;
  }

  if (fetchOffset >= _partCount)
  {
    return 0;
  }

  const unsigned copyCount = std::min(capacity, _partCount - fetchOffset);
  const Part *parts = _parts + fetchOffset;
  const Resource **dst = resources;
  for (unsigned i = 0; i < copyCount; ++i)
  {
    *dst++ = parts->resource;
    ++parts;
  }
  return copyCount;
}


Shape *MeshSet::clone() const
{
  MeshSet *copy = new MeshSet(_partCount);
  onClone(copy);
  return copy;
}


void MeshSet::onClone(MeshSet *copy) const
{
  Shape::onClone(copy);
  if (copy->_partCount != _partCount)
  {
    delete[] copy->_parts;
    if (_partCount)
    {
      copy->_parts = new Part[_partCount];
    }
    else
    {
      copy->_parts = nullptr;
    }
  }
  copy->_ownPartResources = false;
  std::copy(_parts, _parts + _partCount, copy->_parts);
}


void MeshSet::cleanupParts()
{
  if (_ownPartResources && _parts)
  {
    for (unsigned i = 0; i < _partCount; ++i)
    {
      delete _parts[i].resource;
    }
  }

  delete[] _parts;

  _parts = nullptr;
  _ownPartResources = false;
}
