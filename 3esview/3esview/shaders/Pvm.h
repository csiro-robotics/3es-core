//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SHADERS_PVM_H
#define TES_VIEW_SHADERS_PVM_H

#include <3esview/ViewConfig.h>

#include <3esview/util/Enum.h>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix4.h>

namespace tes::view::shaders
{
/// A helper for tracking projection, view and model matrices for shaders.
///
/// Typical usage is to call the set functions as matrices change, which marks particular matrices as dirty. Then
/// before drawing, check the flags to see if a particular matrix is dirty and only update the shader value when dirty.
/// After drawing, call @c clearDirty() to reset the flags.
///
/// Some examples:
///
/// ```c++
/// void draw1(auto &shader, Pvm &pvm, Mesh &mesh)
/// {
///   // Draw case where the shader only supports a single full projection transformation.
///   if (pvm.dirtyPvm())
///   {
///     shader.setProjectionTransform(pvm.pvm());
///   }
///   pvm.clearDirty();
///   shader.draw(mesh);
/// }
///
/// void draw2(auto &shader, Pvm &pvm, Mesh &mesh)
/// {
///   // Draw where model matrix is split out
///   if (pvm.dirtyPv())
///   {
///     shader.setProjectionTransform(pvm.pv());
///   }
///   pvm.clearDirty();
///   // Here we assume the model matrix will be changing each call.
///   shader.setModelMatrix(pvm.model());
///   shader.draw(mesh);
/// }
///
/// void draw3(auto &shader, Pvm &pvm, Mesh &mesh)
/// {
///   // Draw case where the shader combines the three matrices.
///   if (pvm.dirtyProjection())
///   {
///     shader.setProjectionMatrix(pvm.projection());
///   }
///   if (pvm.dirtyView())
///   {
///     shader.setViewMatrix(pvm.view());
///   }
///   if (pvm.dirtyModel())
///   {
///     shader.setModelMatrix(pvm.model());
///   }
///   pvm.clearDirty();
///   shader.draw(mesh);
/// }
/// ```
///
/// From this usage, we can see how the dirty flags are for the user to observe and this class does nothing other than
/// provide the information about what has changed since the last call to @c clearDirty().
class TES_VIEWER_API Pvm
{
public:
  enum class DirtyFlag : unsigned
  {
    Zero = 0,
    Projection = (1u << 0u),
    View = (1u << 1u),
    Model = (1u << 2u),

    PVM = unsigned(Projection) | unsigned(View) | unsigned(Model),
    PV = unsigned(Projection) | unsigned(View),
    VM = unsigned(View) | unsigned(Model),
  };

  const Magnum::Matrix4 &projection() const;
  void setProjection(const Magnum::Matrix4 &matrix);

  const Magnum::Matrix4 &view() const;
  void setView(const Magnum::Matrix4 &matrix);

  const Magnum::Matrix4 &model() const;
  void setModel(const Magnum::Matrix4 &matrix);

  bool dirtyProjection() const;
  bool dirtyView() const;
  bool dirtyModel() const;
  bool dirtyPvm() const;
  bool dirtyPv() const;
  bool dirtyVm() const;

  /// Request the full `projection * view * model` transform.
  /// @return The full PVM transformation.
  Magnum::Matrix4 pvm() const;
  /// Request the `projection * view` transform.
  /// @return The PV transformation.
  Magnum::Matrix4 pv() const;
  /// Request the `view * model` transform.
  /// @return The VM transformation.
  Magnum::Matrix4 vm() const;

  void clearDirty(DirtyFlag flag);
  void clearDirty();

protected:
  Magnum::Matrix4 _projection_matrix;
  Magnum::Matrix4 _view_matrix;
  Magnum::Matrix4 _model_matrix;
  DirtyFlag _flags = DirtyFlag::Zero;
};

TES_ENUM_FLAGS(Pvm::DirtyFlag, unsigned);


inline const Magnum::Matrix4 &Pvm::projection() const
{
  return _projection_matrix;
}


inline void Pvm::setProjection(const Magnum::Matrix4 &matrix)
{
  _projection_matrix = matrix;
  _flags |= DirtyFlag::Projection;
}


inline const Magnum::Matrix4 &Pvm::view() const
{
  return _view_matrix;
}


inline void Pvm::setView(const Magnum::Matrix4 &matrix)
{
  _view_matrix = matrix;
  _flags |= DirtyFlag::View;
}


inline const Magnum::Matrix4 &Pvm::model() const
{
  return _model_matrix;
}


inline void Pvm::setModel(const Magnum::Matrix4 &matrix)
{
  _model_matrix = matrix;
  _flags |= DirtyFlag::Model;
}


inline bool Pvm::dirtyProjection() const
{
  return (_flags & DirtyFlag::Projection) != DirtyFlag::Zero;
}


inline bool Pvm::dirtyView() const
{
  return (_flags & DirtyFlag::View) != DirtyFlag::Zero;
}


inline bool Pvm::dirtyModel() const
{
  return (_flags & DirtyFlag::Model) != DirtyFlag::Zero;
}


inline bool Pvm::dirtyPvm() const
{
  return (_flags & DirtyFlag::PVM) != DirtyFlag::Zero;
}


inline bool Pvm::dirtyPv() const
{
  return (_flags & DirtyFlag::PV) != DirtyFlag::Zero;
}


inline bool Pvm::dirtyVm() const
{
  return (_flags & DirtyFlag::VM) != DirtyFlag::Zero;
}


inline Magnum::Matrix4 Pvm::pvm() const
{
  return _projection_matrix * _view_matrix * _model_matrix;
}


inline Magnum::Matrix4 Pvm::pv() const
{
  return _projection_matrix * _view_matrix;
}


inline Magnum::Matrix4 Pvm::vm() const
{
  return _view_matrix * _model_matrix;
}


inline void Pvm::clearDirty(DirtyFlag flag)
{
  _flags &= ~flag;
}


inline void Pvm::clearDirty()
{
  _flags = DirtyFlag::Zero;
}
}  // namespace tes::view::shaders

#endif  // TES_VIEW_SHADERS_PVM_H
