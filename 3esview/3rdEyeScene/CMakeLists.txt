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
  # Vk
)
# find_package(MagnumExtras CONFIG REQUIRED)
find_package(Threads)

#-------------------------------------------------------------------------------
# Configuration header setup.
#-------------------------------------------------------------------------------

configure_file(ClientConfig.in.h "${CMAKE_CURRENT_BINARY_DIR}/3rdEyeScene/ClientConfig.h")
set(PUBLIC_HEADERS
  # Generated files.
  "${CMAKE_CURRENT_BINARY_DIR}/3rdEyeScene/ClientConfig.h"
)

#-------------------------------------------------------------------------------
# Define the source files for the 3rdEyeScene executable.
#-------------------------------------------------------------------------------
include(sourcelist.cmake)

corrade_add_resource(TesResources resources.conf)

#-------------------------------------------------------------------------------
# Library setup.
#-------------------------------------------------------------------------------
# Define the 3rdEyeScene library.
add_executable(3rdEyeScene ${PUBLIC_HEADERS} ${SOURCES} ${PRIVATE_HEADERS} ${PRIVATE_SOURCES} ${TesResources})
tes_configure_target(3rdEyeScene)

# set_target_properties(3rdEyeScene PROPERTIES FOLDER view)

# Include directories
target_include_directories(3rdEyeScene
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/3rdEyeScene>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
    $<INSTALL_INTERFACE:include/3rdEyeScene>
)

target_link_libraries(3rdEyeScene
  PRIVATE
    3escore
    $<$<BOOL:CMAKE_THREAD_LIBS_INIT>:${CMAKE_THREAD_LIBS_INIT}>
    3esview
    Corrade::Containers
    Magnum::Application
    Magnum::Magnum
    Magnum::Primitives
    Magnum::Shaders
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
#     message(STATUS "Adding doxygen target: 3esdoc")

#     # Create a target to build the documentation.
#     # Here we also setup various documentation variables passed through to the doxyfile configuration.
#     # Each named argument below describes the Doxygen variable it sets.
#     doxygen_create(
#       DOXYFILE 3esdoc/doxyfile.in  # Doxyfile to configure.
#       PROJECT 3es # PROJECT_NAME
#       VERSION ${TES_VERSION}        # PROJECT_NUMBER
#       BRIEF "3rd Eye Scene documentation" # PROJECT_BRIEF
#       OUTPUT_DIR html               # HTML_OUPTUT
#       # CSS <style>.css             # HTML_STYLESHEET
#       PUBLISHER "CSIRO"             # DOCSET_PUBLISHER_NAME
#       PUBLISHER_ID au.csiro         # DOCSET_PUBLISHER_ID
#       PROJECT_ID au.csiro.3es       # DOCSET_BUNDLE_ID, QHP_NAMESPACE, ECLIPSE_DOC_ID
#       PATHS                         # INPUT (RECURSIVE is on)
#         3esdoc  # Must come first to define documentation groups.
#         .
#       EXCLUDE_PATHS                 # EXCLUDE
#         ./private
#       # Where to find source code examples.
#       # EXAMPLE_PATHS <paths>        # EXAMPLE_PATH
#       # Where to find images.
#       IMAGE_PATHS                    # IMAGE_PATH
#         3esdoc/images
#         ${ADDITIONAL_IMAGE_DIRS}
#       )

#     # Setup installation of the generated documentation: source, destination.
#     doxygen_install("${CMAKE_CURRENT_BINARY_DIR}/html" 3es)
#   endif(DOXYGEN_FOUND)
# endif(TES_BUILD_DOXYGEN)

#-------------------------------------------------------------------------------
# IDE source sorting
#-------------------------------------------------------------------------------
source_group("source" FILES "${CMAKE_CURRENT_BINARY_DIR}/3rdEyeScene/ClientConfig.h")
source_group("source" REGULAR_EXPRESSION ".*$")
# source_group("source\\private" REGULAR_EXPRESSION "/private/.*$")
