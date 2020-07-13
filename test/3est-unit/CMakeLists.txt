
set(SOURCES
  3est-collate.cpp
  3est-common.cpp
  3est-common.h
  3est-core.cpp
  3est-shapes.cpp
  3est-vertexbuffer.cpp
)

add_executable(3est-unit ${SOURCES})
set_target_properties(3est-unit PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_link_libraries(3est-unit 3es-core GTest::Main)
target_include_directories(3est-unit
  PRIVATE
    $<TARGET_PROPERTY:3es-core,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:GTest::Main,INTERFACE_INCLUDE_DIRECTORIES>
)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
