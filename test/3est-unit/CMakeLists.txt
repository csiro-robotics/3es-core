
set(SOURCES
  3est-collate.cpp
  3est-common.cpp
  3est-common.h
  3est-core.cpp
  3est-shapes.cpp
  3est-databuffer.cpp
)

add_executable(3est-unit ${SOURCES})
tes_configure_unit_test_target(3est-unit)
target_link_libraries(3est-unit
  PRIVATE
    3es-core
    GTest::Main
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
