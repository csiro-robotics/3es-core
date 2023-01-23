
set(SOURCES
  TestCollate.cpp
  TestCommon.cpp
  TestCommon.h
  TestCore.cpp
  TestDataBuffer.cpp
  TestShapes.cpp
)

add_executable(3est-unit ${SOURCES})
tes_configure_unit_test_target(3est-unit)
target_link_libraries(3est-unit
  PRIVATE
    3escore
    GTest::Main
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
