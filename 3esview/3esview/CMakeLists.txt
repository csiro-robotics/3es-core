#-------------------------------------------------------------------------------
# Project setup. Detects this as the root or may be included as part of another
# project.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# 3esview configuration options.
#-------------------------------------------------------------------------------
find_package(cxxopts CONFIG REQUIRED)
find_package(Corrade CONFIG REQUIRED Containers)
find_package(Magnum CONFIG REQUIRED
  GlfwApplication
  DebugTools
  # MagnumFont
  Primitives
  Shaders
  Text
  # TgaImporter
  # Vk
)
# find_package(MagnumExtras CONFIG REQUIRED)
find_package(ryml CONFIG REQUIRED)
find_package(Threads)
# TODO(KS): Check the vcpkg version of nfd for generaly compability. It seems quite different.
# A submodule or source import may be better for this one.
find_package(unofficial-nativefiledialog CONFIG REQUIRED)

#-------------------------------------------------------------------------------
# Configuration header setup.
#-------------------------------------------------------------------------------
configure_file(ViewConfig.in.h "${CMAKE_CURRENT_BINARY_DIR}/3esview/ViewConfig.h")
set(PUBLIC_HEADERS
  # Generated files.
  "${CMAKE_CURRENT_BINARY_DIR}/3esview/ViewConfig.h"
  "${CMAKE_CURRENT_BINARY_DIR}/3esview/ViewExport.h"
)

#-------------------------------------------------------------------------------
# Define the source files for the 3esview library.
#-------------------------------------------------------------------------------
include(sourcelist.cmake)

#-------------------------------------------------------------------------------
# Library setup.
#-------------------------------------------------------------------------------
# Define the 3esview library.
add_library(3esview ${PUBLIC_HEADERS} ${SOURCES} ${PRIVATE_HEADERS} ${PRIVATE_SOURCES})
tes_configure_target(3esview INCLUDE_PREFIX include/3esview PUBLIC_HEADERS ${PUBLIC_HEADERS})

# Define DLL export header (primarily for MSVC).
include(GenerateExportHeader)
generate_export_header(3esview
      EXPORT_MACRO_NAME TES_VIEWER_API
      NO_EXPORT_MACRO_NAME TES_VIEWER_HIDDEN
      EXPORT_FILE_NAME 3esview/ViewExport.h
      STATIC_DEFINE TES_VIEWER_STATIC
      )

# set_target_properties(3esview PROPERTIES FOLDER view)

# Include directories
target_include_directories(3esview
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/3esview>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(3esview SYSTEM
  PRIVATE
    # Add cxxopts include directories from the import target. It's a header only library so we don't
    # need to link anything or propagate the dependency.
    $<TARGET_PROPERTY:cxxopts::cxxopts,INCLUDE_DIRECTORIES>
)

target_link_libraries(3esview
  PUBLIC
    3escore
    $<$<BOOL:CMAKE_THREAD_LIBS_INIT>:${CMAKE_THREAD_LIBS_INIT}>
  PRIVATE
    # Should only need BUILD_INTERFACE when building a shared library.
    $<BUILD_INTERFACE:Corrade::Containers>
    $<BUILD_INTERFACE:Magnum::Application>
    $<BUILD_INTERFACE:Magnum::Magnum>
    $<BUILD_INTERFACE:Magnum::Primitives>
    $<BUILD_INTERFACE:Magnum::Shaders>
    $<BUILD_INTERFACE:Magnum::Text>
    $<BUILD_INTERFACE:ryml::ryml>
    $<BUILD_INTERFACE:unofficial::nativefiledialog::nfd>)

#-------------------------------------------------------------------------------
# IDE source sorting
#-------------------------------------------------------------------------------
source_group("source" FILES "${CMAKE_CURRENT_BINARY_DIR}/3esview/ViewConfig.h")
source_group("source" REGULAR_EXPRESSION ".*$")
# source_group("source\\private" REGULAR_EXPRESSION "/private/.*$")
