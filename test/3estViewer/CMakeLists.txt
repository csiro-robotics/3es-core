# Choose the appropriate windowless application name.
# Note: these are not guaranteed to work yet. They just illustrate what should
# be used on each platform. The following platforms are expected to work:
# - WIN32
# - LINUX
set(TES_DISABLE_PAINTER_TESTS_INIT OFF)
if(WIN32)
  set(WINDOWLESS_APP WindowlessWglApplication)
elseif(IOS)
  set(WINDOWLESS_APP WindowlessIosApplication)
elseif(APPLE)
  set(WINDOWLESS_APP WindowlessCglApplication)
elseif(UNIX)
  set(WINDOWLESS_APP WindowlessGlxApplication)
  # Just starting up an WindowlessGlxApplication and deleting it raises an exception in a secondary
  # thread, created when the OpenGL context is created. For now, I'll disable the painter tests
  # which use WindowlessGlxApplication to avoid mis reporting unit tests as failed, when the
  # tests themselves pass, but the application crashes afterwards.
  set(TES_DISABLE_PAINTER_TESTS_INIT ON)
else()
  message(FATAL_ERROR "No windowless Magnum application is available for the current platform.")
endif()

option(TES_DISABLE_PAINTER_TESTS
  "Disable painter unit tests? Added when WindowlessGlxApplication was crashing on exit."
  ${TES_DISABLE_PAINTER_TESTS_INIT})

find_package(Corrade CONFIG REQUIRED Containers Main)
find_package(Magnum CONFIG REQUIRED
  "${WINDOWLESS_APP}"
)

configure_file(TestViewerConfig.in.h "${CMAKE_CURRENT_BINARY_DIR}/3estViewer/TestViewerConfig.h")

set(SOURCES
  TestShapes.cpp
  TestUtil.cpp
  TestViewer.cpp
  TestViewer.h
  TestViewerConfig.in.h
)

corrade_add_resource(TesUnitTestResources "${CMAKE_SOURCE_DIR}/3esview/3rdEyeScene/resources.conf")

add_executable(3estViewer ${SOURCES} ${TesUnitTestResources})
tes_configure_unit_test_target(3estViewer GTEST SOURCES ${SOURCES})

target_include_directories(3estViewer
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  )
target_link_libraries(3estViewer
  PRIVATE
    3escore
    3esview
    Magnum::${WINDOWLESS_APP}
    ${TES_MAGNUM_PLUGIN_LIBRARIES}
    GTest::Main
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
