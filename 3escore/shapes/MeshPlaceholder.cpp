//
// author: Kazys Stepanas
//
#include "MeshPlaceholder.h"

using namespace tes;

MeshPlaceholder::MeshPlaceholder(uint32_t id)
  : _id(id)
{}


void MeshPlaceholder::setId(uint32_t newId)
{
  _id = newId;
}


uint32_t MeshPlaceholder::id() const
{
  return _id;
}


Transform MeshPlaceholder::transform() const
{
  return Transform::identity();
}


uint32_t MeshPlaceholder::tint() const
{
  return 0;
}


uint8_t MeshPlaceholder::drawType(int /* stream */) const
{
  return 0;
}


unsigned MeshPlaceholder::vertexCount(int /* stream */) const
{
  return 0;
}


unsigned MeshPlaceholder::indexCount(int /* stream */) const
{
  return 0;
}


DataBuffer MeshPlaceholder::vertices(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer();
}


DataBuffer MeshPlaceholder::indices(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer();
}


DataBuffer MeshPlaceholder::normals(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer();
}


DataBuffer MeshPlaceholder::uvs(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer();
}


DataBuffer MeshPlaceholder::colours(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer();
}


Resource *MeshPlaceholder::clone() const
{
  return new MeshPlaceholder(_id);
}
