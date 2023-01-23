#-------------------------------------------------------------------------------
# Project setup. Detects this as the root or may be included as part of another
# project.
#-------------------------------------------------------------------------------
set(TES_CORE_ONLY Off)
# Project setup if this is being built as a stand alone project.
if(NOT DEFINED CMAKE_PROJECT_NAME OR CMAKE_PROJECT_NAME STREQUAL "3escore")
  project(3escore)

  cmake_minimum_required(VERSION 3.5)

  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  set(TES_CORE_ONLY On)
endif(NOT DEFINED CMAKE_PROJECT_NAME OR CMAKE_PROJECT_NAME STREQUAL "3escore")

if(NOT ConfigPackageLocation)
  set(ConfigPackageLocation lib/cmake/3es)
endif(NOT ConfigPackageLocation)

#-------------------------------------------------------------------------------
# Version setup
#-------------------------------------------------------------------------------
# 3escore version. These are referenced when configuring CoreConfig.h
include(cmake/3es-version.cmake)

#-------------------------------------------------------------------------------
# 3escore configuration options.
#-------------------------------------------------------------------------------
option(TES_ASSERT_ENABLE_DEBUG "Enable assertions in debug builds." ON)
option(TES_ASSERT_ENABLE_RELEASE "Enable assertions in release builds." ON)
option(TES_BUILD_DOXYGEN "Build doxgen documentation?" OFF)

option(TES_ZLIB_OFF "Disable ZLIB usage even if found? Intended for testing." OFF)
set(TES_SOCKETS "custom" CACHE STRING "Select the TCP socket implementation. The 'custom' implementaiton is based on Berkley sockets or Winsock2.")
set_property(CACHE TES_SOCKETS PROPERTY STRINGS custom Qt)

message(STATUS "Using ${TES_SOCKETS} sockets")

# Find supporting libraies.
# ZLib
set(TES_ZLIB 0)
if(NOT DEFINED TES_ZLIB_OFF OR NOT TES_ZLIB_OFF)
  find_package(ZLIB)
  if(ZLIB_FOUND)
    set(TES_ZLIB 1)
  endif(ZLIB_FOUND)
endif(NOT DEFINED TES_ZLIB_OFF OR NOT TES_ZLIB_OFF)

# Qt (for sockets)
if(TES_SOCKETS STREQUAL "custom")
  list(APPEND DOXYGEN_INPUT_LIST "${CMAKE_CURRENT_LIST_DIR}/tcp")
elseif(TES_SOCKETS STREQUAL "Qt")
  find_package(Qt5Network)
else()
  message(FATAL_ERROR "Unknown socket implementation specificed: ${TES_SOCKETS}")
endif()

find_package(Threads)

#-------------------------------------------------------------------------------
# Configuration header setup.
#-------------------------------------------------------------------------------
# Setup information for configuring the 3escore configuration header.
# Set TES_32 and TES_64 to 1 or 0 depending on CMAKE_SIZEOF_VOID_P denoting 32- or 64-bit pointers.
math(EXPR TES_32 "(${CMAKE_SIZEOF_VOID_P} & 4) >> 2")
math(EXPR TES_64 "(${CMAKE_SIZEOF_VOID_P} & 8) >> 3")

if(NOT DEFINED TES_IS_BIG_ENDIAN)
  include(TestBigEndian)
  TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
  if(IS_BIG_ENDIAN)
    set(TES_IS_BIG_ENDIAN 1 CACHE INTERNAL "Big endian target?")
  else(IS_BIG_ENDIAN)
    set(TES_IS_BIG_ENDIAN 0 CACHE INTERNAL "Big endian target?")
  endif(IS_BIG_ENDIAN)
endif(NOT DEFINED TES_IS_BIG_ENDIAN)

configure_file(CoreConfig.in.h "${CMAKE_CURRENT_BINARY_DIR}/3escore/CoreConfig.h")
set(PUBLIC_HEADERS
  # Generated files.
  "${CMAKE_CURRENT_BINARY_DIR}/3escore/CoreConfig.h"
  "${CMAKE_CURRENT_BINARY_DIR}/3escore/CoreExport.h"
)

#-------------------------------------------------------------------------------
# Define the source files for the 3escore library.
#-------------------------------------------------------------------------------
include(sourcelist.cmake)

#-------------------------------------------------------------------------------
# Library setup.
#-------------------------------------------------------------------------------
# Define the 3escore library.
add_library(3escore ${PUBLIC_HEADERS} ${PUBLIC_SHAPE_HEADERS} ${PUBLIC_TESSELLATE_HEADERS} ${SOURCES} ${PRIVATE_HEADERS} ${PRIVATE_SOURCES})
tes_configure_target(3escore)

# Define DLL export header (primarily for MSVC).
include(GenerateExportHeader)
generate_export_header(3escore
      EXPORT_MACRO_NAME _3es_coreAPI
      NO_EXPORT_MACRO_NAME _3es_coreHIDDEN
      EXPORT_FILE_NAME 3escore/CoreExport.h
      STATIC_DEFINE TES_STATIC
      )

# set_target_properties(3escore PROPERTIES FOLDER core)

# Include directories
target_include_directories(3escore
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/3escore>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:include/3escore>
)

target_link_libraries(3escore
  PUBLIC
    $<$<BOOL:CMAKE_THREAD_LIBS_INIT>:${CMAKE_THREAD_LIBS_INIT}>
    $<$<CXX_COMPILER_ID:MSVC>:Ws2_32.lib>
)

if(ZLIB_FOUND AND NOT TES_ZLIB_OFF)
  target_include_directories(3escore PRIVATE SYSTEM "${ZLIB_INCLUDE_DIRS}")
  target_link_libraries(3escore PUBLIC ${ZLIB_LIBRARIES})
endif(ZLIB_FOUND AND NOT TES_ZLIB_OFF)

# Need to explicitly define some compile flags because the target name starts with a number.
if(BUILD_SHARED_LIBS)
  target_compile_definitions(3escore PRIVATE -D_3es_core_EXPORTS)
else(BUILD_SHARED_LIBS)
  target_compile_definitions(3escore PUBLIC -DTES_STATIC)
endif(BUILD_SHARED_LIBS)

# Link dependencies
if(TES_SOCKETS STREQUAL "Qt")
  target_link_libraries(3escore PRIVATE Qt5::Network)
  target_include_directories(3escore PRIVATE "${Qt5Network_INCLUDE_DIRS}")
endif()

#-------------------------------------------------------------------------------
# Doxygen configuration.
#-------------------------------------------------------------------------------
# Doxygen setup.
if(TES_BUILD_DOXYGEN)
  # Include Doxygen helper functions. This also finds the Doxygen package.
  include(cmake/doxygen.cmake)

  set(ADDITIONAL_IMAGE_DIRS)
  # Define UI images path if present.
  if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../3rdEyeScene/Assets/UI/Images")
    list(APPEND ADDITIONAL_IMAGE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../3rdEyeScene/Assets/UI/Images")
  endif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../3rdEyeScene/Assets/UI/Images")

  if(DOXYGEN_FOUND)
    message(STATUS "Adding doxygen target: 3es-doc")

    # Create a target to build the documentation.
    # Here we also setup various documentation variables passed through to the doxyfile configuration.
    # Each named argument below describes the Doxygen variable it sets.
    doxygen_create(
      DOXYFILE 3es-doc/doxyfile.in  # Doxyfile to configure.
      PROJECT 3es # PROJECT_NAME
      VERSION ${TES_VERSION}        # PROJECT_NUMBER
      BRIEF "3rd Eye Scene documentation" # PROJECT_BRIEF
      OUTPUT_DIR html               # HTML_OUPTUT
      # CSS <style>.css             # HTML_STYLESHEET
      PUBLISHER "CSIRO"             # DOCSET_PUBLISHER_NAME
      PUBLISHER_ID au.csiro         # DOCSET_PUBLISHER_ID
      PROJECT_ID au.csiro.3es       # DOCSET_BUNDLE_ID, QHP_NAMESPACE, ECLIPSE_DOC_ID
      PATHS                         # INPUT (RECURSIVE is on)
        3es-doc  # Must come first to define documentation groups.
        .
      EXCLUDE_PATHS                 # EXCLUDE
        ./private
      # Where to find source code examples.
      # EXAMPLE_PATHS <paths>        # EXAMPLE_PATH
      # Where to find images.
      IMAGE_PATHS                    # IMAGE_PATH
        3es-doc/images
        ${ADDITIONAL_IMAGE_DIRS}
      )

    # Setup installation of the generated documentation: source, destination.
    doxygen_install("${CMAKE_CURRENT_BINARY_DIR}/html" 3es)
  endif(DOXYGEN_FOUND)
endif(TES_BUILD_DOXYGEN)

#-------------------------------------------------------------------------------
# Installation.
#-------------------------------------------------------------------------------
# Header installation
install(FILES ${PUBLIC_HEADERS} DESTINATION include/3escore COMPONENT Devel)
install(FILES ${PUBLIC_SHAPE_HEADERS} DESTINATION include/3escore/shapes COMPONENT Devel)
install(FILES ${PUBLIC_TESSELLATE_HEADERS} DESTINATION include/3escore/tessellate COMPONENT Devel)

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

configure_file(cmake/3es-config.in.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/3es-config.cmake"
  COPYONLY
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
source_group("source" FILES "${CMAKE_CURRENT_BINARY_DIR}/3escore/CoreConfig.h")
source_group("source" REGULAR_EXPRESSION ".*$")
source_group("source\\private" REGULAR_EXPRESSION "/private/.*$")
source_group("source\\qt" REGULAR_EXPRESSION "/qt/.*$")
source_group("source\\shapes" REGULAR_EXPRESSION "/shapes/.*$")
source_group("source\\tcp" REGULAR_EXPRESSION "/tcp/.*$")
