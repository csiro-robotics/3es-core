
add_executable(3estTessellate Tessellate.cpp)
tes_configure_target(3estTessellate)
target_link_libraries(3estTessellate
  PRIVATE
    3escore
)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
