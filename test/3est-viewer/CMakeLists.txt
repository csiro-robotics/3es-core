
set(SOURCES
  3est-shapes.cpp
  3est-util.cpp
)

add_executable(3est-viewer ${SOURCES})
tes_configure_unit_test_target(3est-viewer)
target_link_libraries(3est-viewer
  PRIVATE
    3escore
    3esview
    GTest::Main
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
