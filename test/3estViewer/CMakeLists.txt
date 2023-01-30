
set(SOURCES
  TestShapes.cpp
  TestUtil.cpp
)

add_executable(3estViewer ${SOURCES})
tes_configure_unit_test_target(3estViewer GTEST)
target_link_libraries(3estViewer
  PRIVATE
    3escore
    3esview
    GTest::Main
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
