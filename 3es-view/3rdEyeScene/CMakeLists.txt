#-------------------------------------------------------------------------------
# Project setup. Detects this as the root or may be included as part of another
# project.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Version setup
#-------------------------------------------------------------------------------
# include(cmake/3es-version.cmake)

#-------------------------------------------------------------------------------
# 3rdEyeScene configuration options.
#-------------------------------------------------------------------------------
find_package(Corrade CONFIG REQUIRED Containers Main)
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

configure_file(3es-client.in.h "${CMAKE_CURRENT_BINARY_DIR}/3es-client.h")
set(PUBLIC_HEADERS
  # Generated files.
  "${CMAKE_CURRENT_BINARY_DIR}/3es-client.h"
)

#-------------------------------------------------------------------------------
# Define the source files for the 3rdEyeScene executable.
#-------------------------------------------------------------------------------
include(sourcelist.cmake)

corrade_add_resource(tes_RESOURCES resources.conf)

#-------------------------------------------------------------------------------
# Library setup.
#-------------------------------------------------------------------------------
# Define the 3rdEyeScene library.
add_executable(3rdEyeScene ${PUBLIC_HEADERS} ${SOURCES} ${PRIVATE_HEADERS} ${PRIVATE_SOURCES} ${tes_RESOURCES})
set_target_properties(3rdEyeScene PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

# Disable some warnings.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # Disable warning about and/or precedence. Remember when it comes to precedence:
  # - and is equivalent to multiplication
  # - or is equilvanet to addition
  # Now we don't need excessive brackets (parentheses for the Americans)
  target_compile_options(3rdEyeScene PRIVATE "-Wno-logical-op-parentheses")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

# set_target_properties(3rdEyeScene PROPERTIES FOLDER view)

# Setup target versioning.
set_target_properties(3rdEyeScene PROPERTIES
  # Build version.
  VERSION ${TES_VERSION}
  # API version.
  SOVERSION ${TES_VERSION_MAJOR} # API version.
  # Compatibility versioning. Referenced via the COMPATIBLE_INTERFACE_STRING property.
  INTERFACE_3esclient_MAJOR_VERSION ${TES_VERSION_MAJOR}
)

# Help enforce version compability by ensuring all link dependencies expect the same INTERFACE_3esclient_MAJOR_VERSION
set_property(TARGET 3rdEyeScene APPEND PROPERTY COMPATIBLE_INTERFACE_STRING INTERFACE_3esclient_MAJOR_VERSION)

# Include directories
target_include_directories(3rdEyeScene
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:include/3rdEyeScene>
)

if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(3rdEyeScene ${CMAKE_THREAD_LIBS_INIT})
endif(CMAKE_THREAD_LIBS_INIT)

# Need to explicitly define some compile flags because the target name starts with a number.
if(BUILD_SHARED_LIBS)
  target_compile_definitions(3rdEyeScene PRIVATE -D_3es_client_EXPORTS)
else(BUILD_SHARED_LIBS)
  target_compile_definitions(3rdEyeScene PUBLIC -DTES_STATIC)
endif(BUILD_SHARED_LIBS)

# Link dependencies
# if(ZLIB_FOUND AND NOT TES_ZLIB_OFF)
#   target_include_directories(3rdEyeScene PRIVATE "${ZLIB_INCLUDE_DIRS}")
#   target_link_libraries(3rdEyeScene ${ZLIB_LIBRARIES})
# endif(ZLIB_FOUND AND NOT TES_ZLIB_OFF)

target_link_libraries(3rdEyeScene PUBLIC 3es-core)

target_link_libraries(3rdEyeScene PRIVATE
  3es-viewer
  Corrade::Containers
  Magnum::Application
  Magnum::Magnum
  Magnum::Primitives
  Magnum::Shaders)

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
install(TARGETS 3rdEyeScene EXPORT 3es-config-targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

# Install PDB files (MSVC) in part to avoid unsupressable linker warnings.
if(BUILD_SHARED_LIBS)
  install(FILES $<TARGET_PDB_FILE:3rdEyeScene> DESTINATION bin OPTIONAL)
endif(BUILD_SHARED_LIBS)

# Header installation
# install(FILES ${PUBLIC_HEADERS} DESTINATION include/3rdEyeScene COMPONENT Devel)
# install(FILES ${PUBLIC_SHAPE_HEADERS} DESTINATION include/3rdEyeScene/shapes COMPONENT Devel)

#-------------------------------------------------------------------------------
# IDE source sorting
#-------------------------------------------------------------------------------
source_group("source" FILES "${CMAKE_CURRENT_BINARY_DIR}/3es-client.h")
source_group("source" REGULAR_EXPRESSION ".*$")
# source_group("source\\private" REGULAR_EXPRESSION "/private/.*$")
