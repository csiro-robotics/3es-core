#-------------------------------------------------------------------------------
# Project setup. Detects this as the root or may be included as part of another
# project.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Version setup
#-------------------------------------------------------------------------------
# include(cmake/3es-version.cmake)

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
find_package(Threads)

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
tes_configure_target(3esview)

# Define DLL export header (primarily for MSVC).
include(GenerateExportHeader)
generate_export_header(3esview
      EXPORT_MACRO_NAME TES_VIEWER_API
      NO_EXPORT_MACRO_NAME TES_VIEWER_HIDDEN
      EXPORT_FILE_NAME ViewExport.h
      STATIC_DEFINE TES_VIEWER_STATIC
      )

# set_target_properties(3esview PROPERTIES FOLDER view)

# Include directories
target_include_directories(3esview
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/3esview>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/3esview>
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
    # Should only use BUILD_INTERFACE when building a shared library.
    $<BUILD_INTERFACE:Corrade::Containers>
    $<BUILD_INTERFACE:Magnum::Application>
    $<BUILD_INTERFACE:Magnum::Magnum>
    $<BUILD_INTERFACE:Magnum::Primitives>
    $<BUILD_INTERFACE:Magnum::Shaders>
    $<BUILD_INTERFACE:Magnum::Text>
)

#-------------------------------------------------------------------------------
# Doxygen configuration.
#-------------------------------------------------------------------------------
# # Doxygen setup.
# if(TES_BUILD_DOXYGEN)
#   # Include Doxygen helper functions. This also finds the Doxygen package.
#   include(cmake/doxygen.cmake)

#   set(ADDITIONAL_IMAGE_DIRS)
#   # Define UI images path if present.
#   if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../3rdEyeScene/Assets/UI/Images")
#     list(APPEND ADDITIONAL_IMAGE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../3rdEyeScene/Assets/UI/Images")
#   endif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../3rdEyeScene/Assets/UI/Images")

#   if(DOXYGEN_FOUND)
#     message(STATUS "Adding doxygen target: 3es-doc")

#     # Create a target to build the documentation.
#     # Here we also setup various documentation variables passed through to the doxyfile configuration.
#     # Each named argument below describes the Doxygen variable it sets.
#     doxygen_create(
#       DOXYFILE 3es-doc/doxyfile.in  # Doxyfile to configure.
#       PROJECT 3es # PROJECT_NAME
#       VERSION ${TES_VERSION}        # PROJECT_NUMBER
#       BRIEF "3rd Eye Scene documentation" # PROJECT_BRIEF
#       OUTPUT_DIR html               # HTML_OUPTUT
#       # CSS <style>.css             # HTML_STYLESHEET
#       PUBLISHER "CSIRO"             # DOCSET_PUBLISHER_NAME
#       PUBLISHER_ID au.csiro         # DOCSET_PUBLISHER_ID
#       PROJECT_ID au.csiro.3es       # DOCSET_BUNDLE_ID, QHP_NAMESPACE, ECLIPSE_DOC_ID
#       PATHS                         # INPUT (RECURSIVE is on)
#         3es-doc  # Must come first to define documentation groups.
#         .
#       EXCLUDE_PATHS                 # EXCLUDE
#         ./private
#       # Where to find source code examples.
#       # EXAMPLE_PATHS <paths>        # EXAMPLE_PATH
#       # Where to find images.
#       IMAGE_PATHS                    # IMAGE_PATH
#         3es-doc/images
#         ${ADDITIONAL_IMAGE_DIRS}
#       )

#     # Setup installation of the generated documentation: source, destination.
#     doxygen_install("${CMAKE_CURRENT_BINARY_DIR}/html" 3es)
#   endif(DOXYGEN_FOUND)
# endif(TES_BUILD_DOXYGEN)

#-------------------------------------------------------------------------------
# Installation.
#-------------------------------------------------------------------------------
# Header installation
# install(FILES ${PUBLIC_HEADERS} DESTINATION include/3esview COMPONENT Devel)
# install(FILES ${PUBLIC_SHAPE_HEADERS} DESTINATION include/3esview/shapes COMPONENT Devel)

# Setup import scripts.
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/3es-config-version.cmake"
  VERSION ${TES_VERSION}
  COMPATIBILITY AnyNewerVersion
)

export(EXPORT 3es-config-targets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/3es-config-targets.cmake"
  NAMESPACE 3es::
)

install(EXPORT 3es-config-targets
  FILE 3es-config-targets.cmake
  NAMESPACE 3es::
  DESTINATION ${ConfigPackageLocation}
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/3es-config.cmake"
    # "${CMAKE_CURRENT_BINARY_DIR}/3es-config-targets.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/3es-config-version.cmake"
  DESTINATION ${ConfigPackageLocation}
  COMPONENT Devel)

  # Install MSVC runtime libraries. This will also affect the CPack installation.
include(InstallRequiredSystemLibraries)

#-------------------------------------------------------------------------------
# IDE source sorting
#-------------------------------------------------------------------------------
source_group("source" FILES "${CMAKE_CURRENT_BINARY_DIR}/3esview/ViewConfig.h")
source_group("source" REGULAR_EXPRESSION ".*$")
# source_group("source\\private" REGULAR_EXPRESSION "/private/.*$")
