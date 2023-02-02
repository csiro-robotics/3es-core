//
// author Kazys Stepanas
//
// Copyright (c) Kazys Stepanas 2014
//
#ifndef TES_CORE_SERVER_MACROS_H
#define TES_CORE_SERVER_MACROS_H

#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif  // __clang__
#pragma GCC diagnostic ignored "-Waddress"
#pragma GCC diagnostic ignored "-Wnonnull-compare"
#endif  // __GNUC__

// Temporarily disable the macro interface until new MeshShape API is resolved
#ifdef TES_ENABLE

#include "CoreConfig.h"

#include "ConnectionMonitor.h"
#include "Server.h"
#include "ServerUtil.h"

#include "Colour.h"
#include "CoordinateFrame.h"
#include "DataBuffer.h"
#include "Feature.h"
#include "MeshMessages.h"
#include "Messages.h"
#include "Transform.h"
#include "shapes/Shapes.h"

#include <array>

//-----------------------------------------------------------------------------
// General macros.
//-----------------------------------------------------------------------------

/// @ingroup tescpp
/// @defgroup tesmacros 3rd Eye Scene Macro Interface
/// The 3<sup>rd</sup> Eye Scene macro interface provides a way of instrumenting your code with
/// 3<sup>rd</sup> Eye Scene directives, while being able to conditionally remove these directives
/// from selected builds. The macros are enabled if @c TES_ENABLE is defined when including
/// <tt>3esservermacros.h</tt>. Otherwise the macros remove the code contained in their brackets.

/// @ingroup tesmacros
/// Enable @p statement if TES is enabled.
///
/// The statement is completely removed when TES is not enabled.
/// @param statement The code statement to execute.
#define TES_STMT(statement) statement

/// @ingroup tesmacros
/// Begins an if statement with condition, but only if TES is enabled. Otherwise the macro is
/// <tt>if (false)</tt>
/// @param condition The if statement condition.
#define TES_IF(condition) if (condition)

/// @ingroup tesmacros
/// Colour from RGB.
/// @param r Red channel value [0, 255].
/// @param g Green channel value [0, 255].
/// @param b Blue channel value [0, 255].
#define TES_RGB(r, g, b) tes::Colour(r, g, b)
/// @ingroup tesmacros
/// Colour from RGBA.
/// @param r Red channel value [0, 255].
/// @param g Green channel value [0, 255].
/// @param b Blue channel value [0, 255].
/// @param a Alpha channel value [0, 255].
#define TES_RGBA(r, g, b, a) tes::Colour(r, g, b, a)

/// @ingroup tesmacros
/// Colour by name.
/// @param name a member of @p tes::Colour::Predefined.
#define TES_COLOUR(name) tes::Colour(tes::Colour::name)

/// @ingroup tesmacros
/// Colour by predefined index.
/// @param index A valid value within @p tes::Colour::Predefined.
#define TES_COLOUR_I(index) tes::Colour(static_cast<tes::Colour::NamedColour>(index))

/// @ingroup tesmacros
/// Colour by name with alpha.
/// @param name a member of @p tes::Colour::Predefined.
/// @param a Alpha channel value [0, 255].
#define TES_COLOUR_A(name, a) tes::Colour(tes::Colour::name, a)

/// @ingroup tesmacros
/// A wrapper for constructing a @c DataBuffer around the given arguments.
#define TES_BUFFER(...) tes::DataBuffer(__VA_ARGS__)

//-----------------------------------------------------------------------------
// Server setup macros
//-----------------------------------------------------------------------------

/// @ingroup tesmacros
/// Exposes details of a category to connected clients.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param _name A null terminated, UTF-8 string name for the category.
/// @param _categoryId ID of the category being named [0, 65535].
/// @param _parentId ID of the parent category, to support category trees. Zero for none. [0, 65535]
/// @param _active Default the category to the active state (true/false)?
#define TES_CATEGORY(server, _name, _categoryId, _parentId, _active)                        \
  if (server)                                                                               \
  {                                                                                         \
    tes::CategoryNameMessage msg;                                                           \
    msg.category_id = static_cast<uint16_t>(_categoryId);                                   \
    msg.parent_id = static_cast<uint16_t>(_parentId);                                       \
    msg.default_active = (_active) ? 1 : 0;                                                 \
    const size_t nameLen = (_name != nullptr) ? strlen(_name) : 0u;                         \
    msg.name_length = (uint16_t)((nameLen <= 0xffffu) ? nameLen : 0xffffu);                 \
    msg.name = _name;                                                                       \
    tes::sendMessage(*(server), tes::MtCategory, tes::CategoryNameMessage::MessageId, msg); \
  }

/// @ingroup tesmacros
/// A helper macro used to declare a @p Server pointer and compile out when TES is not enabled.
/// Initialises @p server as a @p Server variable with a null value.
/// @param server The variable name for the @c Server object.
#define TES_SERVER_DECL(server) tes::Server *server = nullptr

/// @ingroup tesmacros
/// A helper macro used to declare and initialise @p ServerSettings and compile out when TES is
/// not enabled.
/// @param settings The variable name for the @p ServerSettings.
/// @param ... Additional arguments passed to the @p ServerSettings constructor.
#define TES_SETTINGS(settings, ...) tes::ServerSettings settings = tes::ServerSettings(__VA_ARGS__)
/// @ingroup tesmacros
/// Initialise a default @p ServerInfoMessage and assign the specified @p CoordinateFrame.
///
/// The time unit details for @p info can be initialise using @c TES_SERVER_INFO_TIME()
/// @see @c initDefaultServerInfo()
/// @param info Variable name for the @c ServerInfoMessage structure.
/// @param info_coordinate_frame The server's @c CoordinateFrame value.
#define TES_SERVER_INFO(info, info_coordinate_frame) \
  tes::ServerInfoMessage info;                       \
  tes::initDefaultServerInfo(&info);                 \
  info.coordinate_frame = info_coordinate_frame;

/// @ingroup tesmacros
/// Initialise the time unit details of a @c ServerInfoMessage.
/// @param info the @c ServerInfoMessage structure variable.
/// @param time_unit The @c ServerInfoMessage::time_unit value to set.
/// @param default_frame_time The @c ServerInfoMessage::default_frame_time value to set.
#define TES_SERVER_INFO_TIME(info, time_unit, default_frame_time) \
  info.time_unit = time_unit;                                     \
  info.default_frame_time = default_frame_time;

/// @ingroup tesmacros
/// Initialise @p server to a new @c Server object with the given @c ServerSettings and
/// @c ServerInfoMessage.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param settings The @c ServerSettings structure to initialise the server with.
/// @param info The @c ServerInfoMessage structure to initialise the server with.
#define TES_SERVER_CREATE(server, settings, info) server = tes::Server::create(settings, info);

/// @ingroup tesmacros
/// Start the given @c Server in the given mode (synchronous or asynchronous).
///
/// After this call, the server can accept connections.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param mode The server mode: @c ConnectionMonitor::Synchronous or @c
/// ConnectionMonitor::Asynchronous.
#define TES_SERVER_START(server, mode)          \
  if (server)                                   \
  {                                             \
    (server)->connectionMonitor()->start(mode); \
  }

/// @ingroup tesmacros
/// Call to update the server flushing the frame and potentially monitoring new connections.
///
/// This update macro performs the following update commands:
/// - Call @c Server::updateFrame()
/// - Update connections, accepting new and expiring old.
/// - Updates any pending cache transfers.
///
/// Any additional macro arguments are passed to @c Server::updateFrame(). At the very least
/// a delta time value must be passed (floating point, in seconds). This should be zero when
/// using TES for algorithm debugging, or a valid time delta in real-time debugging.
///
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ... Arguments for @c Server::updateFrame()
#define TES_SERVER_UPDATE(server, ...)                               \
  if (server)                                                        \
  {                                                                  \
    (server)->updateTransfers(0);                                    \
    (server)->updateFrame(__VA_ARGS__);                              \
    tes::ConnectionMonitor *_conMon = (server)->connectionMonitor(); \
    if (_conMon->mode() == tes::ConnectionMonitor::Synchronous)      \
    {                                                                \
      _conMon->monitorConnections();                                 \
    }                                                                \
    _conMon->commitConnections();                                    \
  }

/// @ingroup tesmacros
/// Wait for the server to be ready to accept incoming connections.
/// This blocks until at least one connection is established up to @p timems milliseconds.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param timems The wait time out to wait for (milliseconds).
#define TES_SERVER_START_WAIT(server, timems)                                   \
  if ((server) && (server)->connectionMonitor()->waitForConnection(timems) > 0) \
  {                                                                             \
    (server)->connectionMonitor()->commitConnections();                         \
  }

/// @ingroup tesmacros
/// Set the connection callback via @c ConnectionMonitor::setConnectionCallback().
#define TES_SET_CONNECTION_CALLBACK(server, ...)                       \
  if (server)                                                          \
  {                                                                    \
    (server)->connectionMonitor()->setConnectionCallback(__VA_ARGS__); \
  }

/// @ingroup tesmacros
/// Stop the server. The server is closed and disposed and is no longer valid for use after
/// this call.
/// Note the @p server argument must be a pointer as it is first checked against null, then
/// cleared to @c nullptr.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
#define TES_SERVER_STOP(server) \
  if (server)                   \
  {                             \
    (server)->close();          \
    (server)->dispose();        \
    (server) = nullptr;         \
  }

/// @ingroup tesmacros
/// Open a local file stream to filename. All messages are streamed to this file.
/// Note there is no way to close the file using the macro interface.
#define TES_LOCAL_FILE_STREAM(server, filename)              \
  if (server)                                                \
  {                                                          \
    (server)->connectionMonitor()->openFileStream(filename); \
  }

/// @ingroup tesmacros
/// Check if @p server is enabled.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
#define TES_ACTIVE(server) ((server) != nullptr && (server)->active())

/// @ingroup tesmacros
/// Enable/disable @p server.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param _active @c true to activate, @c false to deactivate.
#define TES_SET_ACTIVE(server, _active) \
  if (server)                           \
  {                                     \
    (server)->setActive(_active)        \
  }

/// @ingroup tesmacros
/// Check if a feature is enabled using @c checkFeature().
/// @param feature The feature to check for.
#define TES_FEATURE(feature) tes::checkFeature(feature)
/// @ingroup tesmacros
/// Get the flag for a feature.
/// @param feature The feature identifier.
#define TES_FEATURE_FLAG(feature) tes::featureFlag(feature)
/// @ingroup tesmacros
/// Check if the given set of features are enabled using @c checkFeatures().
/// @param featureFlags The flags to check for.
#define TES_FEATURES(featureFlags) tes::checkFeatures(featureFlags)

/// @ingroup tesmacros
/// Execute @c expression if @p featureFlags are all present using @c checkFeatures().
/// @param featureFlags The flags to require before executing @p expression.
/// @param expression The code statement or expression to execute if @c checkFeatures() passes.
#define TES_IF_FEATURES(featureFlags, expression) \
  if (tes::checkFeatures(featureFlags))           \
  {                                               \
    expression;                                   \
  }

//-----------------------------------------------------------------------------
// Shape macros
//-----------------------------------------------------------------------------

/// @ingroup tesmacros
/// Adds a reference to the given @c resource. See @c tes::Connection::referenceResource(). Adds the
/// resource to the server if there is no existing resource.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param resource A pointer to the resource.
#define TES_REFERENCE_RESOURCE(server, resource) \
  if (server)                                    \
  {                                              \
    (server)->referenceResource(resource);       \
  }

/// @ingroup tesmacros
/// Releases a reference to the given @c resource. See @c tes::Connection::referenceResource().
/// Destroys the resource if this is the final reference.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param resource A pointer to the resource.
#define TES_RELEASE_RESOURCE(server, resource) \
  if (server)                                  \
  {                                            \
    (server)->releaseResource(resource);       \
  }

/// @ingroup tesmacros
/// Makes a stack declaration of a placeholder mesh resource.
/// Primarily for use with @c TES_REFERENCE_RESOURCE(), @c TES_RELEASE_RESOURCE and @c
/// TES_MESHSET_END().
/// @param id The mesh resource ID to proxy.
#define TES_MESH_PLACEHOLDER(id) tes::MeshPlaceholder(id)

/// @ingroup tesmacros
/// Solid arrow.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Arrow() constructor.
#define TES_ARROW(server, colour, ...)                           \
  if (server)                                                    \
  {                                                              \
    (server)->create(tes::Arrow(__VA_ARGS__).setColour(colour)); \
  }
/// @ingroup tesmacros
/// Transparent arrow.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Arrow() constructor.
#define TES_ARROW_T(server, colour, ...)                                              \
  if (server)                                                                         \
  {                                                                                   \
    (server)->create(tes::Arrow(__VA_ARGS__).setColour(colour).setTransparent(true)); \
  }
/// @ingroup tesmacros
/// Wireframe arrow.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Arrow() constructor.
#define TES_ARROW_W(server, colour, ...)                                            \
  if (server)                                                                       \
  {                                                                                 \
    (server)->create(tes::Arrow(__VA_ARGS__).setColour(colour).setWireframe(true)); \
  }

#define _TES_MAKE_AABB(c, s, minExt, maxExt)    \
  tes::V3Arg iext(minExt), mext(maxExt);        \
  tes::Vector3f c = 0.5f * (iext.v3 + mext.v3); \
  tes::Vector3f s = (mext.v3 - iext.v3)

#define TES_BOX_AABB(server, colour, id, minExt, maxExt)    \
  if (server)                                               \
  {                                                         \
    _TES_MAKE_AABB(c, s, minExt, maxExt);                   \
    (server)->create(tes::Box(id, c, s).setColour(colour)); \
  }

#define TES_BOX_AABB_T(server, colour, id, minExt, maxExt)                       \
  if (server)                                                                    \
  {                                                                              \
    _TES_MAKE_AABB(c, s, minExt, maxExt);                                        \
    (server)->create(tes::Box(id, c, s).setColour(colour).setTransparent(true)); \
  }

#define TES_BOX_AABB_W(server, colour, id, minExt, maxExt)                     \
  if (server)                                                                  \
  {                                                                            \
    _TES_MAKE_AABB(c, s, minExt, maxExt);                                      \
    (server)->create(tes::Box(id, c, s).setColour(colour).setWireframe(true)); \
  }

/// @ingroup tesmacros
/// Solid box.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Box() constructor.
#define TES_BOX(server, colour, ...)                           \
  if (server)                                                  \
  {                                                            \
    (server)->create(tes::Box(__VA_ARGS__).setColour(colour)); \
  }
/// @ingroup tesmacros
/// Transparent box.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Box() constructor.
#define TES_BOX_T(server, colour, ...)                                              \
  if (server)                                                                       \
  {                                                                                 \
    (server)->create(tes::Box(__VA_ARGS__).setColour(colour).setTransparent(true)); \
  }
/// @ingroup tesmacros
/// Wireframe box.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Box() constructor.
#define TES_BOX_W(server, colour, ...)                                            \
  if (server)                                                                     \
  {                                                                               \
    (server)->create(tes::Box(__VA_ARGS__).setColour(colour).setWireframe(true)); \
  }

/// @ingroup tesmacros
/// Solid capsule.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Capsule() constructor.
#define TES_CAPSULE(server, colour, ...)                           \
  if (server)                                                      \
  {                                                                \
    (server)->create(tes::Capsule(__VA_ARGS__).setColour(colour)); \
  }
/// @ingroup tesmacros
/// Transparent capsule.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Capsule() constructor.
#define TES_CAPSULE_T(server, colour, ...)                                              \
  if (server)                                                                           \
  {                                                                                     \
    (server)->create(tes::Capsule(__VA_ARGS__).setColour(colour).setTransparent(true)); \
  }
/// @ingroup tesmacros
/// Wireframe capsule.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Capsule() constructor.
#define TES_CAPSULE_W(server, colour, ...)                                            \
  if (server)                                                                         \
  {                                                                                   \
    (server)->create(tes::Capsule(__VA_ARGS__).setColour(colour).setWireframe(true)); \
  }

/// @ingroup tesmacros
/// Solid cone.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Cone() constructor.
#define TES_CONE(server, colour, ...)                           \
  if (server)                                                   \
  {                                                             \
    (server)->create(tes::Cone(__VA_ARGS__).setColour(colour)); \
  }
/// @ingroup tesmacros
/// Transparent cone.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Cone() constructor.
#define TES_CONE_T(server, colour, ...)                                              \
  if (server)                                                                        \
  {                                                                                  \
    (server)->create(tes::Cone(__VA_ARGS__).setColour(colour).setTransparent(true)); \
  }
/// @ingroup tesmacros
/// Wireframe cone.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Cone() constructor.
#define TES_CONE_W(server, colour, ...)                                            \
  if (server)                                                                      \
  {                                                                                \
    (server)->create(tes::Cone(__VA_ARGS__).setColour(colour).setWireframe(true)); \
  }

/// @ingroup tesmacros
/// Solid cylinder.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Cylinder() constructor.
#define TES_CYLINDER(server, colour, ...)                           \
  if (server)                                                       \
  {                                                                 \
    (server)->create(tes::Cylinder(__VA_ARGS__).setColour(colour)); \
  }
/// @ingroup tesmacros
/// Transparent cylinder.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Cylinder() constructor.
#define TES_CYLINDER_T(server, colour, ...)                                              \
  if (server)                                                                            \
  {                                                                                      \
    (server)->create(tes::Cylinder(__VA_ARGS__).setColour(colour).setTransparent(true)); \
  }
/// @ingroup tesmacros
/// Wireframe cylinder.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Cylinder() constructor.
#define TES_CYLINDER_W(server, colour, ...)                                            \
  if (server)                                                                          \
  {                                                                                    \
    (server)->create(tes::Cylinder(__VA_ARGS__).setColour(colour).setWireframe(true)); \
  }

/// @ingroup tesmacros
/// Render a set of lines.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_LINES(server, colour, ...)                                               \
  if (server)                                                                        \
  {                                                                                  \
    (server)->create(tes::MeshShape(tes::DtLines, ##__VA_ARGS__).setColour(colour)); \
  }

/// @ingroup tesmacros
/// Render a set of lines, calling @c MeshShape::expandVertices().
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_LINES_E(server, colour, ...)                                               \
  if (server)                                                                          \
  {                                                                                    \
    (server)->create(                                                                  \
      tes::MeshShape(tes::DtLines, ##__VA_ARGS__).expandVertices().setColour(colour)); \
  }

/// @ingroup tesmacros
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param v0 Vertex of the first line end point.
/// @param v1 Second vertex for the line starting at @p v0.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_LINE(server, colour, id, v0, v1, ...)                                        \
  if (server)                                                                            \
  {                                                                                      \
    const std::array<tes::Vector3d, 2> _line = { tes::Vector3d(v0), tes::Vector3d(v1) }; \
    tes::MeshShape shape(tes::DtLines, id, tes::DataBuffer(_line), ##__VA_ARGS__);       \
    shape.setColour(colour);                                                             \
    (server)->create(shape);                                                             \
  }

/// @ingroup tesmacros
/// Render a complex mesh.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ... Additional arguments follow, passed to @p MeshSet() constructor.
#define TES_MESHSET(server, ...)                 \
  if (server)                                    \
  {                                              \
    (server)->create(tes::MeshSet(__VA_ARGS__)); \
  }

/// @ingroup tesmacros
/// Solid plane.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Plane() constructor.
#define TES_PLANE(server, colour, ...)                           \
  if (server)                                                    \
  {                                                              \
    (server)->create(tes::Plane(__VA_ARGS__).setColour(colour)); \
  }
/// @ingroup tesmacros
/// Transparent plane.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Plane() constructor.
#define TES_PLANE_T(server, colour, ...)                                              \
  if (server)                                                                         \
  {                                                                                   \
    (server)->create(tes::Plane(__VA_ARGS__).setColour(colour).setTransparent(true)); \
  }
/// @ingroup tesmacros
/// Wireframe plane.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Plane() constructor.
#define TES_PLANE_W(server, colour, ...)                                            \
  if (server)                                                                       \
  {                                                                                 \
    (server)->create(tes::Plane(__VA_ARGS__).setColour(colour).setWireframe(true)); \
  }

/// @ingroup tesmacros
/// Render a point cloud.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p PointCloudShape() constructor.
#define TES_POINTCLOUDSHAPE(server, colour, ...)                           \
  if (server)                                                              \
  {                                                                        \
    (server)->create(tes::PointCloudShape(__VA_ARGS__).setColour(colour)); \
  }

/// @ingroup tesmacros
/// Render a small set of points.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_POINTS(server, colour, ...)                                               \
  if (server)                                                                         \
  {                                                                                   \
    (server)->create(tes::MeshShape(tes::DtPoints, ##__VA_ARGS__).setColour(colour)); \
  }

/// @ingroup tesmacros
/// Render a small set of points with per point colours.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colours A @c uint32_t array of colours. The number of elements must match the number of
/// points pass in ...
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_POINTS_C(server, colours, ...)                                              \
  if (server)                                                                           \
  {                                                                                     \
    (server)->create(tes::MeshShape(tes::DtPoints, ##__VA_ARGS__).setColours(colours)); \
  }

/// @ingroup tesmacros
/// Render a small set of points, calling @c MeshShape::expandVertices().
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_POINTS_E(server, colour, ...)                                               \
  if (server)                                                                           \
  {                                                                                     \
    (server)->create(                                                                   \
      tes::MeshShape(tes::DtPoints, ##__VA_ARGS__).expandVertices().setColour(colour)); \
  }

/// @ingroup tesmacros
/// Solid pose.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Pose() constructor.
#define TES_POSE(server, colour, ...)                           \
  if (server)                                                   \
  {                                                             \
    (server)->create(tes::Pose(__VA_ARGS__).setColour(colour)); \
  }
/// @ingroup tesmacros
/// Transparent pose.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Pose() constructor.
#define TES_POSE_T(server, colour, ...)                                              \
  if (server)                                                                        \
  {                                                                                  \
    (server)->create(tes::Pose(__VA_ARGS__).setColour(colour).setTransparent(true)); \
  }
/// @ingroup tesmacros
/// Wireframe pose.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Pose() constructor.
#define TES_POSE_W(server, colour, ...)                                            \
  if (server)                                                                      \
  {                                                                                \
    (server)->create(tes::Pose(__VA_ARGS__).setColour(colour).setWireframe(true)); \
  }

/// @ingroup tesmacros
/// Solid sphere.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Sphere() constructor.
#define TES_SPHERE(server, colour, ...)                           \
  if (server)                                                     \
  {                                                               \
    (server)->create(tes::Sphere(__VA_ARGS__).setColour(colour)); \
  }
/// @ingroup tesmacros
/// Transparent sphere.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Sphere() constructor.
#define TES_SPHERE_T(server, colour, ...)                                              \
  if (server)                                                                          \
  {                                                                                    \
    (server)->create(tes::Sphere(__VA_ARGS__).setColour(colour).setTransparent(true)); \
  }
/// @ingroup tesmacros
/// Wireframe sphere.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Sphere() constructor.
#define TES_SPHERE_W(server, colour, ...)                                            \
  if (server)                                                                        \
  {                                                                                  \
    (server)->create(tes::Sphere(__VA_ARGS__).setColour(colour).setWireframe(true)); \
  }

/// @ingroup tesmacros
/// Solid star.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Star() constructor.
#define TES_STAR(server, colour, ...)                           \
  if (server)                                                   \
  {                                                             \
    (server)->create(tes::Star(__VA_ARGS__).setColour(colour)); \
  }
/// @ingroup tesmacros
/// Transparent star.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Star() constructor.
#define TES_STAR_T(server, colour, ...)                                              \
  if (server)                                                                        \
  {                                                                                  \
    (server)->create(tes::Star(__VA_ARGS__).setColour(colour).setTransparent(true)); \
  }
/// @ingroup tesmacros
/// Wireframe star.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Star() constructor.
#define TES_STAR_W(server, colour, ...)                                            \
  if (server)                                                                      \
  {                                                                                \
    (server)->create(tes::Star(__VA_ARGS__).setColour(colour).setWireframe(true)); \
  }

/// @ingroup tesmacros
/// Render 2D text in screen space. Range is from (0, 0) top left to (1, 1) bottom right. Z ignored.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Text2D() constructor.
#define TES_TEXT2D_SCREEN(server, colour, ...)                    \
  if (server)                                                     \
  {                                                               \
    (server)->create(tes::Text2D(__VA_ARGS__).setColour(colour)); \
  }
/// @ingroup tesmacros
/// Render 2D text with a 3D world location.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Text2D() constructor.
#define TES_TEXT2D_WORLD(server, colour, ...)                                           \
  if (server)                                                                           \
  {                                                                                     \
    (server)->create(tes::Text2D(__VA_ARGS__).setInWorldSpace(true).setColour(colour)); \
  }

/// @ingroup tesmacros
/// Render 3D text.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Text3D() constructor.
#define TES_TEXT3D(server, colour, ...)                           \
  if (server)                                                     \
  {                                                               \
    (server)->create(tes::Text3D(__VA_ARGS__).setColour(colour)); \
  }

/// @ingroup tesmacros
/// Render 3D text, always facing the screen.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p Text3D() constructor.
#define TES_TEXT3D_FACING(server, colour, ...)                                         \
  if (server)                                                                          \
  { (server)->create(tes::Text3D(__VA_ARGS__).setScreenFacing(true).setColour(colour); \
  }

/// @ingroup tesmacros
/// Triangles shape.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_TRIANGLES(server, colour, ...)                 \
  if (server)                                              \
  {                                                        \
    tes::MeshShape shape(tes::DtTriangles, ##__VA_ARGS__); \
    shape.setColour(colour);                               \
    (server)->create(shape);                               \
  }

/// @ingroup tesmacros
/// Triangles shape, calling @c MeshShape::expandVertices().
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_TRIANGLES_E(server, colour, ...)               \
  if (server)                                              \
  {                                                        \
    tes::MeshShape shape(tes::DtTriangles, ##__VA_ARGS__); \
    shape.expandVertices().setColour(colour);              \
    (server)->create(shape);                               \
  }

/// @ingroup tesmacros
/// Triangles shape with lighting (_N to calculate normals).
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_TRIANGLES_N(server, colour, ...)               \
  if (server)                                              \
  {                                                        \
    tes::MeshShape shape(tes::DtTriangles, ##__VA_ARGS__); \
    shape.setCalculateNormals(true).setColour(colour);     \
    (server)->create(shape);                               \
  }

/// @ingroup tesmacros
/// Triangles shape with lighting (_N to calculate normals), calling @c MeshShape::expandVertices().
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_TRIANGLES_NE(server, colour, ...)                           \
  if (server)                                                           \
  {                                                                     \
    tes::MeshShape shape(tes::DtTriangles, ##__VA_ARGS__);              \
    shape.expandVertices().setCalculateNormals(true).setColour(colour); \
    (server)->create(shape);                                            \
  }

/// @ingroup tesmacros
/// Triangles wireframe shape.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_TRIANGLES_W(server, colour, ...)               \
  if (server)                                              \
  {                                                        \
    tes::MeshShape shape(tes::DtTriangles, ##__VA_ARGS__); \
    shape.setWireframe(true);                              \
    shape.setColour(colour);                               \
    (server)->create(shape);                               \
  }

/// @ingroup tesmacros
/// Triangles wireframe shape, calling @c MeshShape::expandVertices().
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_TRIANGLES_WE(server, colour, ...)              \
  if (server)                                              \
  {                                                        \
    tes::MeshShape shape(tes::DtTriangles, ##__VA_ARGS__); \
    shape.expandVertices().setWireframe(true);             \
    shape.setColour(colour);                               \
    (server)->create(shape);                               \
  }

/// @ingroup tesmacros
/// Triangles transparent shape.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_TRIANGLES_T(server, colour, ...)               \
  if (server)                                              \
  {                                                        \
    tes::MeshShape shape(tes::DtTriangles, ##__VA_ARGS__); \
    shape.setTransparent(true);                            \
    shape.setColour(colour);                               \
    (server)->create(shape);                               \
  }

/// @ingroup tesmacros
/// Triangles transparent shape, calling @c MeshShape::expandVertices()
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_TRIANGLES_TE(server, colour, ...)              \
  if (server)                                              \
  {                                                        \
    tes::MeshShape shape(tes::DtTriangles, ##__VA_ARGS__); \
    shape.expandVertices().setTransparent(true);           \
    shape.setColour(colour);                               \
    (server)->create(shape);                               \
  }

/// @ingroup tesmacros
/// Single triangle.
///
/// Vertices are specified as any type which can be used as a constructor argument to @c Vector3f.
/// Generally <tt>const float *</tt> is recommended.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param id The triangle's @c Id.
/// @param v0 First triangle vertex: castable to a @c Vector3f (such as <tt>const float *</tt>).
/// @param v1 Second triangle vertex.
/// @param v2 Third triangle vertex.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_TRIANGLE(server, colour, id, v0, v1, v2, ...)                             \
  if (server)                                                                         \
  {                                                                                   \
    const std::array<tes::Vector3f, 3> _tri = { tes::Vector3f(v0), tes::Vector3f(v1), \
                                                tes::Vector3f(v2) };                  \
    tes::MeshShape shape(tes::DtTriangles, id, tes::DataBuffer(_tri), ##__VA_ARGS__); \
    shape.setColour(colour).setTwoSided(true);                                        \
    (server)->create(shape);                                                          \
  }

/// @ingroup tesmacros
/// Single wireframe triangle.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param id The triangle's @c Id.
/// @param v0 A triangle vertex.
/// @param v1 A triangle vertex.
/// @param v2 A triangle vertex.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_TRIANGLE_W(server, colour, id, v0, v1, v2, ...)                           \
  if (server)                                                                         \
  {                                                                                   \
    const std::array<tes::Vector3f, 3> _tri = { tes::Vector3f(v0), tes::Vector3f(v1), \
                                                tes::Vector3f(v2) };                  \
    tes::MeshShape shape(tes::DtTriangles, id, tes::DataBuffer(_tri), ##__VA_ARGS__); \
    shape.setColour(colour);                                                          \
    shape.setWireframe(true);                                                         \
    (server)->create(shape);                                                          \
  }

/// @ingroup tesmacros
/// Single transparent triangle.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param id The triangle's @c Id.
/// @param v0 A triangle vertex.
/// @param v1 A triangle vertex.
/// @param v2 A triangle vertex.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor.
#define TES_TRIANGLE_T(server, colour, id, v0, v1, v2, ...)                            \
  {                                                                                    \
    const tes::Vector3f std::array<, 3> _tri = { tes::Vector3f(v0), tes::Vector3f(v1), \
                                                 tes::Vector3f(v2) };                  \
    tes::MeshShape shape(tes::DtTriangles, id, tes::DataBuffer(_tri), ##__VA_ARGS__);  \
    shape.setColour(colour);                                                           \
    shape.setTransparent(true).setTwoSided(true);                                      \
    (server)->create(shape);                                                           \
  }

/// @ingroup tesmacros
/// Render a set of voxels. Vertices represent voxel centres, normals are extents.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param colour The colour to apply to the shape.
/// @param resolution The length of the voxel edge. Only supports cubic voxels.
/// @param ... Additional arguments follow, passed to @p MeshShape() constructor. Vertices and
/// normals required.
#define TES_VOXELS(server, colour, resolution, ...)                        \
  if (server)                                                              \
  {                                                                        \
    (server)->create(tes::MeshShape(tes::DtVoxels, ##__VA_ARGS__)          \
                       .setUniformNormal(tes::Vector3f(0.5f * resolution)) \
                       .setColour(colour));                                \
  }

/// @ingroup tesmacros
/// Destroy arrow with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_ARROW_END(server, id)      \
  if (server)                          \
  {                                    \
    (server)->destroy(tes::Arrow(id)); \
  }
/// @ingroup tesmacros
/// Destroy box with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_BOX_END(server, id)      \
  if (server)                        \
  {                                  \
    (server)->destroy(tes::Box(id)); \
  }
/// @ingroup tesmacros
/// Destroy pose with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_POSE_END(server, id)      \
  if (server)                         \
  {                                   \
    (server)->destroy(tes::Pose(id)); \
  }
/// @ingroup tesmacros
/// Destroy capsule with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_CAPSULE_END(server, id)      \
  if (server)                            \
  {                                      \
    (server)->destroy(tes::Capsule(id)); \
  }
/// @ingroup tesmacros
/// Destroy cone with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_CONE_END(server, id)      \
  if (server)                         \
  {                                   \
    (server)->destroy(tes::Cone(id)); \
  }
/// @ingroup tesmacros
/// Destroy cylinder with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_CYLINDER_END(server, id)      \
  if (server)                             \
  {                                       \
    (server)->destroy(tes::Cylinder(id)); \
  }
/// @ingroup tesmacros
/// Destroy lines with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_LINES_END(server, id)                        \
  if (server)                                            \
  {                                                      \
    (server)->destroy(tes::MeshShape(tes::DtLines, id)); \
  }
/// @ingroup tesmacros
/// Destroy mesh with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
/// @param resource The mesh resource associated with the set. Only supports one mesh.
///       Must be a pointer type : <tt>tes::MeshResource *</tt>
#define TES_MESHSET_END(server, id, resource)      \
  if (server)                                      \
  {                                                \
    (server)->destroy(tes::MeshSet(resource, id)); \
  }
/// @ingroup tesmacros
/// Destroy plane with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_PLANE_END(server, id)      \
  if (server)                          \
  {                                    \
    (server)->destroy(tes::Plane(id)); \
  }
/// @ingroup tesmacros
/// Destroy point cloud with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param cloud The @c MeshResource (e.g., @c PointCloud) containing the point vertex data.
/// @param id The ID of the shape to destroy.
#define TES_POINTCLOUDSHAPE_END(server, cloud, id)      \
  if (server)                                           \
  {                                                     \
    (server)->destroy(tes::PointCloudShape(cloud, id)); \
  }
/// @ingroup tesmacros
/// Destroy point set with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_POINTS_END(server, id)                        \
  if (server)                                             \
  {                                                       \
    (server)->destroy(tes::MeshShape(tes::DtPoints, id)); \
  }
/// @ingroup tesmacros
/// Destroy sphere with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_SPHERE_END(server, id)      \
  if (server)                           \
  {                                     \
    (server)->destroy(tes::Sphere(id)); \
  }
/// @ingroup tesmacros
/// Destroy star with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_STAR_END(server, id)      \
  if (server)                         \
  {                                   \
    (server)->destroy(tes::Star(id)); \
  }
/// @ingroup tesmacros
/// Destroy 2D text with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_TEXT2D_END(server, id)          \
  if (server)                               \
  {                                         \
    (server)->destroy(tes::Text2D("", id)); \
  }
/// @ingroup tesmacros
/// Destroy 3D text with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_TEXT3D_END(server, id)          \
  if (server)                               \
  {                                         \
    (server)->destroy(tes::Text3D("", id)); \
  }
/// @ingroup tesmacros
/// Destroy triangle or triangles with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_TRIANGLES_END(server, id)                        \
  if (server)                                                \
  {                                                          \
    (server)->destroy(tes::MeshShape(tes::DtTriangles, id)); \
  }
/// @ingroup tesmacros
/// Destroy arrow with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_TRIANGLE_END(server, id)                         \
  if (server)                                                \
  {                                                          \
    (server)->destroy(tes::MeshShape(tes::DtTriangles, id)); \
  }
/// @ingroup tesmacros
/// Destroy voxel set with @p id.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param id The ID of the shape to destroy.
#define TES_VOXELS_END(server, id)                        \
  if (server)                                             \
  {                                                       \
    (server)->destroy(tes::MeshShape(tes::DtVoxels, id)); \
  }


//-----------------------------------------------------------------------------
// Shape update macros
//-----------------------------------------------------------------------------
/// @ingroup tesmacros
/// Send a position update message for a shape.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ShapeType The class of the shape to update. E.g., @c tes::Box
/// @param objectID The ID of the object to update.
/// @param pos The new position. A @c V3Arg compatible argument.
#define TES_POS_UPDATE(server, ShapeType, objectID, pos)                                           \
  if (server)                                                                                      \
  {                                                                                                \
    (server)->update(                                                                              \
      tes::ShapeType(objectID, 0).setPosition(pos).setFlags(tes::UFUpdateMode | tes::UFPosition)); \
  }

/// @ingroup tesmacros
/// Send an update message for a shape, updating object rotation.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ShapeType The class of the shape to update. E.g., @c tes::Box
/// @param objectID The ID of the object to update.
/// @param quaternion The updated quaternion rotation. A @c QuaternionArg compatible argument.
#define TES_ROT_UPDATE(server, ShapeType, objectID, quaternion)         \
  if (server)                                                           \
  {                                                                     \
    (server)->update(tes::ShapeType(objectID, 0)                        \
                       .setRotation(quaternion)                         \
                       .setFlags(tes::UFUpdateMode | tes::UFRotation)); \
  }

/// @ingroup tesmacros
/// Send an update message for a shape, updating scale.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ShapeType The class of the shape to update. E.g., @c tes::Box
/// @param objectID The ID of the object to update.
/// @param scale The new object scale. A @c V3Arg compatible argument.
#define TES_SCALE_UPDATE(server, ShapeType, objectID, scale)                                   \
  if (server)                                                                                  \
  {                                                                                            \
    (server)->update(                                                                          \
      tes::ShapeType(objectID, 0).setScale(scale).setFlags(tes::UFUpdateMode | tes::UFScale)); \
  }

/// @ingroup tesmacros
/// Send an update message for a shape, updating colour.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ShapeType The class of the shape to update. E.g., @c tes::Box
/// @param objectID The ID of the object to update.
/// @param colour The new object @c Colour.
#define TES_COLOUR_UPDATE(server, ShapeType, objectID, colour)                                    \
  if (server)                                                                                     \
  {                                                                                               \
    (server)->update(                                                                             \
      tes::ShapeType(objectID, 0).setColour(colour).setFlags(tes::UFUpdateMode | tes::UFColour)); \
  }

/// @ingroup tesmacros
/// Send an update message for a shape, updating colour.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ShapeType The class of the shape to update. E.g., @c tes::Box
/// @param objectID The ID of the object to update.
/// @param colour The new object @c Colour.
#define TES_COLOR_UPDATE(server, ShapeType, objectID, colour)                                     \
  if (server)                                                                                     \
  {                                                                                               \
    (server)->update(                                                                             \
      tes::ShapeType(objectID, 0).setColour(colour).setFlags(tes::UFUpdateMode | tes::UFColour)); \
  }

/// @ingroup tesmacros
/// Send an update message for a shape, updating position and rotation.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ShapeType The class of the shape to update. E.g., @c tes::Box
/// @param objectID The ID of the object to update.
/// @param pos The new position. A @c V3Arg compatible argument.
/// @param quaternion The updated quaternion rotation. A @c QuaternionArg compatible argument.
#define TES_POSROT_UPDATE(server, ShapeType, objectID, pos, quaternion)                   \
  if (server)                                                                             \
  {                                                                                       \
    (server)->update(tes::ShapeType(objectID, 0)                                          \
                       .setPosition(pos)                                                  \
                       .setRotation(quaternion)                                           \
                       .setFlags(tes::UFUpdateMode | tes::UFPosition | tes::UFRotation)); \
  }

/// @ingroup tesmacros
/// Send an update message for a shape, updating position and scale.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ShapeType The class of the shape to update. E.g., @c tes::Box
/// @param objectID The ID of the object to update.
/// @param pos The new position. A @c V3Arg compatible argument.
/// @param scale The new object scale. A @c V3Arg compatible argument.
#define TES_POSSCALE_UPDATE(server, ShapeType, objectID, pos, scale)                   \
  if (server)                                                                          \
  {                                                                                    \
    (server)->update(tes::ShapeType(objectID, 0)                                       \
                       .setPosition(pos)                                               \
                       .setScale(scale)                                                \
                       .setFlags(tes::UFUpdateMode | tes::UFPosition | tes::UFScale)); \
  }

/// @ingroup tesmacros
/// Send an update message for a shape, updating rotation and scale.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ShapeType The class of the shape to update. E.g., @c tes::Box
/// @param objectID The ID of the object to update.
/// @param quaternion The updated quaternion rotation. A @c QuaternionArg compatible argument.
/// @param scale The new object scale. A @c V3Arg compatible argument.
#define TES_ROTSCALE_UPDATE(server, ShapeType, objectID, quaternion, scale)            \
  if (server)                                                                          \
  {                                                                                    \
    (server)->update(tes::ShapeType(objectID, 0)                                       \
                       .setRotation(quaternion)                                        \
                       .setScale(scale)                                                \
                       .setFlags(tes::UFUpdateMode | tes::UFRotation | tes::UFScale)); \
  }

/// @ingroup tesmacros
/// Send an update message for a shape, updating position, rotation and scale.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ShapeType The class of the shape to update. E.g., @c tes::Box
/// @param objectID The ID of the object to update.
/// @param pos The new position. A @c V3Arg compatible argument.
/// @param quaternion The updated quaternion rotation. A @c QuaternionArg compatible argument.
/// @param scale The new object scale. A @c V3Arg compatible argument.
#define TES_PRS_UPDATE(server, ShapeType, objectID, pos, quaternion, scale)               \
  if (server)                                                                             \
  {                                                                                       \
    (server)->update(                                                                     \
      tes::ShapeType(objectID, 0)                                                         \
        .setPosition(pos)                                                                 \
        .setRotation(quaternion)                                                          \
        .setScale(scale)                                                                  \
        .setFlags(tes::UFUpdateMode | tes::UFPosition | tes::UFRotation | tes::UFScale)); \
  }

/// @ingroup tesmacros
/// Send an update message for a shape, updating position, rotation and colour.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ShapeType The class of the shape to update. E.g., @c tes::Box
/// @param objectID The ID of the object to update.
/// @param pos The new position. A @c V3Arg compatible argument.
/// @param quaternion The updated quaternion rotation. A @c QuaternionArg compatible argument.
/// @param colour The new object @c Colour.
#define TES_PRC_UPDATE(server, ShapeType, objectID, pos, quaternion, colour)               \
  if (server)                                                                              \
  {                                                                                        \
    (server)->update(                                                                      \
      tes::ShapeType(objectID, 0)                                                          \
        .setPosition(pos)                                                                  \
        .setRotation(quaternion)                                                           \
        .setColour(colour)                                                                 \
        .setFlags(tes::UFUpdateMode | tes::UFPosition | tes::UFRotation | tes::UFColour)); \
  }

/// @ingroup tesmacros
/// Send an update message for a shape, updating position, scale and colour.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ShapeType The class of the shape to update. E.g., @c tes::Box
/// @param objectID The ID of the object to update.
/// @param pos The new position. A @c V3Arg compatible argument.
/// @param scale The new object scale. A @c V3Arg compatible argument.
/// @param colour The new object @c Colour.
#define TES_PSC_UPDATE(server, ShapeType, objectID, pos, scale, colour)                 \
  if (server)                                                                           \
  {                                                                                     \
    (server)->update(                                                                   \
      tes::ShapeType(objectID, 0)                                                       \
        .setPosition(pos)                                                               \
        .setScale(scale)                                                                \
        .setColour(colour)                                                              \
        .setFlags(tes::UFUpdateMode | tes::UFPosition | tes::UFScale | tes::UFColour)); \
  }

/// @ingroup tesmacros
/// Send an update message for a shape, updating rotation, scale and colour.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ShapeType The class of the shape to update. E.g., @c tes::Box
/// @param objectID The ID of the object to update.
/// @param quaternion The updated quaternion rotation. A @c QuaternionArg compatible argument.
/// @param scale The new object scale. A @c V3Arg compatible argument.
/// @param colour The new object @c Colour.
#define TES_RSC_UPDATE(server, ShapeType, objectID, quaternion, scale, colour)          \
  if (server)                                                                           \
  {                                                                                     \
    (server)->update(                                                                   \
      tes::ShapeType(objectID, 0)                                                       \
        .setRotation(quaternion)                                                        \
        .setScale(scale)                                                                \
        .setColour(colour)                                                              \
        .setFlags(tes::UFUpdateMode | tes::UFRotation | tes::UFScale | tes::UFColour)); \
  }

/// @ingroup tesmacros
/// Send an update message for a shape, updating all transform and colour attributes.
/// @param server The @c Server or @c Connection object. Must be a pointer type.
/// @param ShapeType The class of the shape to update. E.g., @c tes::Box
/// @param objectID The ID of the object to update.
/// @param pos The new position. A @c V3Arg compatible argument.
/// @param quaternion The updated quaternion rotation. A @c QuaternionArg compatible argument.
/// @param scale The new object scale. A @c V3Arg compatible argument.
/// @param colour The new object @c Colour.
#define TES_PRSC_UPDATE(server, ShapeType, objectID, pos, quaternion, scale, colour) \
  if (server)                                                                        \
  {                                                                                  \
    (server)->update(tes::ShapeType(objectID, 0)                                     \
                       .setPosition(pos)                                             \
                       .setRotation(quaternion)                                      \
                       .setScale(scale)                                              \
                       .setColour(colour));                                          \
  }

#else  // !TES_ENABLE

namespace tes
{
/// Empty function to suppress penantic warnings
constexpr inline void noop()
{}
}  // namespace tes

#define TES_STMT(statement) tes::noop()
#define TES_IF(condition) if (false)
#define TES_RGB(r, g, b) tes::noop()
#define TES_RGBA(r, g, b, a) tes::noop()
#define TES_COLOUR(name) tes::noop()
#define TES_COLOUR_I(index) tes::noop()
#define TES_COLOUR_A(name, a) tes::noop()
#define TES_BUFFER(...) tes::noop()

#define TES_CATEGORY(server, ...) tes::noop()
#define TES_SERVER_DECL(server) tes::noop()
#define TES_SETTINGS(server, ...) tes::noop()
#define TES_SERVER_INFO(server, ...) tes::noop()
#define TES_SERVER_INFO_TIME(server, ...) tes::noop()
#define TES_SERVER_CREATE(server, ...) tes::noop()
#define TES_SERVER_START(server, ...) tes::noop()
#define TES_SERVER_START_WAIT(server, ...) tes::noop()
#define TES_SET_CONNECTION_CALLBACK(...) tes::noop()
#define TES_SERVER_UPDATE(server, ...) tes::noop()
#define TES_SERVER_STOP(server) tes::noop()
#define TES_LOCAL_FILE_STREAM(server, ...) tes::noop()
#define TES_ACTIVE(server) false
#define TES_SET_ACTIVE(server, ...) tes::noop()

#define TES_FEATURE(feature) false
#define TES_FEATURE_FLAG(feature) 0
#define TES_FEATURES(featureFlags) tes::noop()
#define TES_IF_FEATURES(featureFlags, ...) tes::noop()

#define TES_REFERENCE_RESOURCE(server, ...) tes::noop()
#define TES_RELEASE_RESOURCE(server, ...) tes::noop()
#define TES_MESH_PLACEHOLDER(id) tes::noop()

#define TES_ARROW(server, ...) tes::noop()
#define TES_ARROW_T(server, ...) tes::noop()
#define TES_ARROW_W(server, ...) tes::noop()
#define TES_BOX_AABB(server, ...) tes::noop()
#define TES_BOX_AABB_T(server, ...) tes::noop()
#define TES_BOX_AABB_W(server, ...) tes::noop()
#define TES_BOX(server, ...) tes::noop()
#define TES_BOX_T(server, ...) tes::noop()
#define TES_BOX_W(server, ...) tes::noop()
#define TES_CAPSULE(server, ...) tes::noop()
#define TES_CAPSULE_T(server, ...) tes::noop()
#define TES_CAPSULE_W(server, ...) tes::noop()
#define TES_CONE(server, ...) tes::noop()
#define TES_CONE_T(server, ...) tes::noop()
#define TES_CONE_W(server, ...) tes::noop()
#define TES_CYLINDER(server, ...) tes::noop()
#define TES_CYLINDER_T(server, ...) tes::noop()
#define TES_CYLINDER_W(server, ...) tes::noop()
#define TES_LINES(server, ...) tes::noop()
#define TES_LINES_E(server, ...) tes::noop()
#define TES_LINE(server, ...) tes::noop()
#define TES_MESHSET(server, ...) tes::noop()
#define TES_PLANE(server, ...) tes::noop()
#define TES_PLANE_T(server, ...) tes::noop()
#define TES_PLANE_W(server, ...) tes::noop()
#define TES_POINTCLOUDSHAPE(server, ...) tes::noop()
#define TES_POINTS(server, ...) tes::noop()
#define TES_POINTS_C(server, ...) tes::noop()
#define TES_POINTS_E(server, ...) tes::noop()
#define TES_POSE(server, ...) tes::noop()
#define TES_POSE_T(server, ...) tes::noop()
#define TES_POSE_W(server, ...) tes::noop()
#define TES_SPHERE(server, ...) tes::noop()
#define TES_SPHERE_T(server, ...) tes::noop()
#define TES_SPHERE_W(server, ...) tes::noop()
#define TES_STAR(server, ...) tes::noop()
#define TES_STAR_T(server, ...) tes::noop()
#define TES_STAR_W(server, ...) tes::noop()
#define TES_TEXT2D_SCREEN(server, ...) tes::noop()
#define TES_TEXT2D_WORLD(server, ...) tes::noop()
#define TES_TEXT3D(server, ...) tes::noop()
#define TES_TEXT3D_FACING(server, ...) tes::noop()
#define TES_TRIANGLES(server, ...) tes::noop()
#define TES_TRIANGLES_E(server, ...) tes::noop()
#define TES_TRIANGLES_N(server, ...) tes::noop()
#define TES_TRIANGLES_NE(server, ...) tes::noop()
#define TES_TRIANGLES_W(server, ...) tes::noop()
#define TES_TRIANGLES_WE(server, ...) tes::noop()
#define TES_TRIANGLES_T(server, ...) tes::noop()
#define TES_TRIANGLES_TE(server, ...) tes::noop()
#define TES_TRIANGLE(server, ...) tes::noop()
#define TES_TRIANGLE_W(server, ...) tes::noop()
#define TES_TRIANGLE_T(server, ...) tes::noop()
#define TES_VOXELS(server, ...) tes::noop()

#define TES_ARROW_END(server, ...) tes::noop()
#define TES_BOX_END(server, ...) tes::noop()
#define TES_CAPSULE_END(server, ...) tes::noop()
#define TES_CONE_END(server, ...) tes::noop()
#define TES_CYLINDER_END(server, ...) tes::noop()
#define TES_LINES_END(server, ...) tes::noop()
#define TES_MESHSET_END(server, ...) tes::noop()
#define TES_PLANE_END(server, ...) tes::noop()
#define TES_POINTCLOUDSHAPE_END(server, ...) tes::noop()
#define TES_POINTS_END(server, ...) tes::noop()
#define TES_POSE_END(server, ...) tes::noop()
#define TES_VOXELS_END(server, ...) tes::noop()
#define TES_SPHERE_END(server, ...) tes::noop()
#define TES_STAR_END(server, ...) tes::noop()
#define TES_TEXT2D_END(server, ...) tes::noop()
#define TES_TEXT3D_END(server, ...) tes::noop()
#define TES_TRIANGLES_END(server, ...) tes::noop()
#define TES_TRIANGLE_END(server, ...) tes::noop()
#define TES_VOXELS_END(server, ...) tes::noop()

#define TES_POS_UPDATE(server, ...) tes::noop()
#define TES_ROT_UPDATE(server, ...) tes::noop()
#define TES_SCALE_UPDATE(server, ...) tes::noop()
#define TES_COLOUR_UPDATE(server, ...) tes::noop()
#define TES_COLOR_UPDATE(server, ...) tes::noop()
#define TES_POSROT_UPDATE(server, ...) tes::noop()
#define TES_POSSCALE_UPDATE(server, ...) tes::noop()
#define TES_ROTSCALE_UPDATE(server, ...) tes::noop()
#define TES_PRS_UPDATE(server, ...) tes::noop()
#define TES_PRC_UPDATE(server, ...) tes::noop()
#define TES_PSC_UPDATE(server, ...) tes::noop()
#define TES_RSC_UPDATE(server, ...) tes::noop()
#define TES_PRSC_UPDATE(server, ...) tes::noop()

#endif  // TES_ENABLE

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__

#endif  // TES_CORE_SERVER_MACROS_H
