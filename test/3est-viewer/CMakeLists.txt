
set(SOURCES
  3est-util.cpp
)

add_executable(3est-viewer ${SOURCES})
set_target_properties(3est-viewer PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_link_libraries(3est-viewer 3es-viewer 3es-core GTest::Main)
target_include_directories(3est-viewer
  PRIVATE
    $<TARGET_PROPERTY:3es-core,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:GTest::Main,INTERFACE_INCLUDE_DIRECTORIES>
)

add_test(NAME 3est-viewer COMMAND 3est-viewer --gtest_output=xml:test-reports/)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
