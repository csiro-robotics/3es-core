#-------------------------------------------------------------------------------
# Project setup. Detects this as the root or may be included as part of another
# project.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Version setup
#-------------------------------------------------------------------------------
# include(cmake/3es-version.cmake)

#-------------------------------------------------------------------------------
# 3es-viewer configuration options.
#-------------------------------------------------------------------------------
find_package(Corrade CONFIG REQUIRED Containers)
find_package(Magnum CONFIG REQUIRED
  GlfwApplication
  DebugTools
  # MagnumFont
  Primitives
  Shaders
  Text
  # TgaImporter
  Vk
)
# find_package(MagnumExtras CONFIG REQUIRED)
find_package(Threads)

#-------------------------------------------------------------------------------
# Configuration header setup.
#-------------------------------------------------------------------------------
configure_file(3es-viewer.in.h "${CMAKE_CURRENT_BINARY_DIR}/3es-viewer.h")
set(PUBLIC_HEADERS
  # Generated files.
  "${CMAKE_CURRENT_BINARY_DIR}/3es-viewer.h"
  "${CMAKE_CURRENT_BINARY_DIR}/3es-viewer-export.h"
)

#-------------------------------------------------------------------------------
# Define the source files for the 3es-viewer library.
#-------------------------------------------------------------------------------
include(sourcelist.cmake)

#-------------------------------------------------------------------------------
# Library setup.
#-------------------------------------------------------------------------------
# Define the 3es-viewer library.
# add_library(3es-viewer ${PUBLIC_HEADERS} ${SOURCES} ${PRIVATE_HEADERS} ${PRIVATE_SOURCES})
# add_executable(3es-viewer ${PUBLIC_HEADERS} ${SOURCES} ${PRIVATE_HEADERS} ${PRIVATE_SOURCES})
add_library(3es-viewer ${PUBLIC_HEADERS} ${SOURCES} ${PRIVATE_HEADERS} ${PRIVATE_SOURCES})
set_target_properties(3es-viewer PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

# Disable some warnings.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # Disable warning about and/or precedence. Remember when it comes to precedence:
  # - and is equivalent to multiplication
  # - or is equilvanet to addition
  # Now we don't need excessive brackets (parentheses for the Americans)
  target_compile_options(3es-viewer PRIVATE "-Wno-logical-op-parentheses")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

# Define DLL export header (primarily for MSVC).
include(GenerateExportHeader)
generate_export_header(3es-viewer
      EXPORT_MACRO_NAME _3es_viewerAPI
      NO_EXPORT_MACRO_NAME _3es_viewerHIDDEN
      EXPORT_FILE_NAME 3es-viewer-export.h
      STATIC_DEFINE TES_STATIC
      )

# set_target_properties(3es-viewer PROPERTIES FOLDER view)

# Setup target versioning.
set_target_properties(3es-viewer PROPERTIES
  # Build version.
  VERSION ${TES_VERSION}
  # API version.
  SOVERSION ${TES_VERSION_MAJOR} # API version.
  # Compatibility versioning. Referenced via the COMPATIBLE_INTERFACE_STRING property.
  INTERFACE_3esclient_MAJOR_VERSION ${TES_VERSION_MAJOR}
)

# Help enforce version compability by ensuring all link dependencies expect the same INTERFACE_3esclient_MAJOR_VERSION
set_property(TARGET 3es-viewer APPEND PROPERTY COMPATIBLE_INTERFACE_STRING INTERFACE_3esclient_MAJOR_VERSION)

# Include directories
target_include_directories(3es-viewer
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:include/3es-viewer>
)

if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(3es-viewer ${CMAKE_THREAD_LIBS_INIT})
endif(CMAKE_THREAD_LIBS_INIT)

target_link_libraries(3es-viewer PUBLIC 3es-core)

target_link_libraries(3es-viewer PRIVATE
  $<BUILD_INTERFACE:Corrade::Containers>
  $<BUILD_INTERFACE:Magnum::Application>
  $<BUILD_INTERFACE:Magnum::Magnum>
  $<BUILD_INTERFACE:Magnum::Primitives>
  $<BUILD_INTERFACE:Magnum::Shaders>
  $<BUILD_INTERFACE:Magnum::Vk>
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
# Binary installation
install(TARGETS 3es-viewer EXPORT 3es-config-targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

# Install PDB files (MSVC) in part to avoid unsupressable linker warnings.
if(BUILD_SHARED_LIBS)
  install(FILES $<TARGET_PDB_FILE:3es-viewer> DESTINATION bin OPTIONAL)
endif(BUILD_SHARED_LIBS)

# Header installation
# install(FILES ${PUBLIC_HEADERS} DESTINATION include/3es-viewer COMPONENT Devel)
# install(FILES ${PUBLIC_SHAPE_HEADERS} DESTINATION include/3es-viewer/shapes COMPONENT Devel)

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
source_group("source" FILES "${CMAKE_CURRENT_BINARY_DIR}/3es-viewer.h")
source_group("source" REGULAR_EXPRESSION ".*$")
# source_group("source\\private" REGULAR_EXPRESSION "/private/.*$")
