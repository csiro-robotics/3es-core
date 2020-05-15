//
// author: Kazys Stepanas
//
#include "3esmeshset.h"

#include "3esmeshmessages.h"
#include "3esmeshplaceholder.h"
#include "3esrotation.h"

#include <algorithm>

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
    memset(_parts, 0, sizeof(*_parts) * partCount.i);
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

  ObjectAttributes attr;
  uint32_t partId;
  uint16_t numberOfParts = uint16_t(partCount());

  memset(&attr, 0, sizeof(attr));

  stream.writeElement(numberOfParts);

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

    attr.position[0] = static_cast<decltype(attr.position[0])>(part.transform.position()[0]);
    attr.position[1] = static_cast<decltype(attr.position[1])>(part.transform.position()[1]);
    attr.position[2] = static_cast<decltype(attr.position[2])>(part.transform.position()[2]);
    attr.rotation[0] = static_cast<decltype(attr.rotation[0])>(part.transform.rotation()[0]);
    attr.rotation[1] = static_cast<decltype(attr.rotation[1])>(part.transform.rotation()[1]);
    attr.rotation[2] = static_cast<decltype(attr.rotation[2])>(part.transform.rotation()[2]);
    attr.rotation[3] = static_cast<decltype(attr.rotation[3])>(part.transform.rotation()[3]);
    attr.scale[0] = static_cast<decltype(attr.scale[0])>(part.transform.scale()[0]);
    attr.scale[1] = static_cast<decltype(attr.scale[1])>(part.transform.scale()[1]);
    attr.scale[2] = static_cast<decltype(attr.scale[2])>(part.transform.scale()[2]);
    attr.colour = part.colour.c;

    stream.writeElement(partId);
    attr.write(stream);
  }

  return true;
}


bool MeshSet::readCreate(PacketReader &stream)
{
  if (!Shape::readCreate(stream))
  {
    return false;
  }

  ObjectAttributes attr;
  uint32_t partId = 0;
  uint16_t numberOfParts = 0;

  bool ok = true;

  memset(&attr, 0, sizeof(attr));

  ok = ok && stream.readElement(numberOfParts) == sizeof(numberOfParts);

  if (ok && numberOfParts > _partCount)
  {
    cleanupParts();

    _parts = new Part[numberOfParts];
    _ownPartResources = true;

    memset(_parts, 0, sizeof(*_parts) * numberOfParts);

    _partCount = numberOfParts;
  }

  for (unsigned i = 0; i < _partCount; ++i)
  {
    attr.position[0] = static_cast<decltype(attr.position[0])>(_parts[i].transform.position[0]);
    attr.position[1] = static_cast<decltype(attr.position[1])>(_parts[i].transform.position[1]);
    attr.position[2] = static_cast<decltype(attr.position[2])>(_parts[i].transform.position[2]);
    attr.rotation[0] = static_cast<decltype(attr.rotation[0])>(_parts[i].transform.rotation[0]);
    attr.rotation[1] = static_cast<decltype(attr.rotation[1])>(_parts[i].transform.rotation[1]);
    attr.rotation[2] = static_cast<decltype(attr.rotation[2])>(_parts[i].transform.rotation[2]);
    attr.rotation[3] = static_cast<decltype(attr.rotation[3])>(_parts[i].transform.rotation[3]);
    attr.scale[0] = static_cast<decltype(attr.scale[0])>(_parts[i].transform.scale[0]);
    attr.scale[1] = static_cast<decltype(attr.scale[1])>(_parts[i].transform.scale[1]);
    attr.scale[2] = static_cast<decltype(attr.scale[2])>(_parts[i].transform.scale[2]);

    ok = ok && stream.readElement(partId) == sizeof(partId);
    ok = ok && attr.read(stream);

    if (ok)
    {
      _parts[i].transform = prsTransform(Vector3f(attr.position), Quaternionf(attr.rotation), Vector3f(attr.scale));
      // We can only reference dummy meshes here.
      _parts[i].resource = new MeshPlaceholder(partId);
      _parts[i].colour = Colour(attr.colour);
    }
  }

  return ok;
}


unsigned MeshSet::enumerateResources(const Resource **resources, unsigned capacity, unsigned fetchOffset) const
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
  memcpy(copy->_parts, _parts, sizeof(*_parts) * _partCount);
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
