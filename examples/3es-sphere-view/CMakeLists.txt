add_executable(3es-sphere-view 3es-sphere-view.cpp)
set_target_properties(3es-sphere-view PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_link_libraries(3es-sphere-view 3es-core)
target_include_directories(3es-sphere-view PRIVATE $<TARGET_PROPERTY:3es-core,INCLUDE_DIRECTORIES>)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
