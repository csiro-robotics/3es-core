#-------------------------------------------------------------------------------
# Project setup. Detects this as the root or may be included as part of another
# project.
#-------------------------------------------------------------------------------

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
# Seems there's a slight comability issue between ImGui and the Magnum integration thereof.
# The Magnum integration looks for a target ImGui::ImGui, while it's actually imgui::imgui.
# We find the package here and alias it resolve the issue.
find_package(ImGui CONFIG REQUIRED)
add_library(ImGui::ImGui ALIAS imgui::imgui)
find_package(MagnumIntegration REQUIRED
  ImGui
)
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
    MagnumIntegration::ImGui
    imgui::imgui
    ${TES_MAGNUM_PLUGIN_LIBRARIES}
)

#-------------------------------------------------------------------------------
# IDE source sorting
#-------------------------------------------------------------------------------
source_group("source" FILES "${CMAKE_CURRENT_BINARY_DIR}/3rdEyeScene/ClientConfig.h")
source_group("source" REGULAR_EXPRESSION ".*$")
# source_group("source\\private" REGULAR_EXPRESSION "/private/.*$")
