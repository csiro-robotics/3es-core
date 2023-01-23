add_executable(3esSphereView SphereView.cpp)
tes_configure_target(3esSphereView)
target_link_libraries(3esSphereView
  PRIVATE
    3escore
)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
