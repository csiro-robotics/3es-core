
set(SOURCES
  TestCollate.cpp
  TestCommon.cpp
  TestCommon.h
  TestCore.cpp
  TestDataBuffer.cpp
  TestShapes.cpp
  TestStream.cpp
)

add_executable(3estUnit ${SOURCES})
tes_configure_unit_test_target(3estUnit GTEST)
target_link_libraries(3estUnit
  PRIVATE
    3escore
    GTest::Main
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
