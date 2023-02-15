//
// author: Kazys Stepanas
//
#include "MeshPlaceholder.h"

namespace tes
{
MeshPlaceholder::MeshPlaceholder(uint32_t id)
  : _id(id)
{}


void MeshPlaceholder::setId(uint32_t new_id)
{
  _id = new_id;
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


float MeshPlaceholder::drawScale(int /* stream */) const
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
  return {};
}


DataBuffer MeshPlaceholder::indices(int stream) const
{
  TES_UNUSED(stream);
  return {};
}


DataBuffer MeshPlaceholder::normals(int stream) const
{
  TES_UNUSED(stream);
  return {};
}


DataBuffer MeshPlaceholder::uvs(int stream) const
{
  TES_UNUSED(stream);
  return {};
}


DataBuffer MeshPlaceholder::colours(int stream) const
{
  TES_UNUSED(stream);
  return {};
}


std::shared_ptr<Resource> MeshPlaceholder::clone() const
{
  return std::make_shared<MeshPlaceholder>(_id);
}
}  // namespace tes
