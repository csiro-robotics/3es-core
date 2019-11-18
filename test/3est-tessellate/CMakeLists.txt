
add_executable(3est-tessellate 3es-tessellate.cpp)
set_target_properties(3est-tessellate PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_link_libraries(3est-tessellate 3es-core)
target_include_directories(3est-tessellate PRIVATE $<TARGET_PROPERTY:3es-core,INCLUDE_DIRECTORIES>)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
