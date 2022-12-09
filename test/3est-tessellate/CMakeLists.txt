
add_executable(3est-tessellate 3es-tessellate.cpp)
tes_configure_target(3est-tessellate)
target_link_libraries(3est-tessellate
  PRIVATE
    3es-core
)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
