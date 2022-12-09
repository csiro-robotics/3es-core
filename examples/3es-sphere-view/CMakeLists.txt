add_executable(3es-sphere-view 3es-sphere-view.cpp)
tes_configure_target(3es-sphere-view)
target_link_libraries(3es-sphere-view
  PRIVATE
    3es-core
)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
