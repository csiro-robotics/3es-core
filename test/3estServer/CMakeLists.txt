
add_executable(3estServer ServerTest.cpp)
tes_configure_target(3estServer)
target_link_libraries(3estServer
  PRIVATE
    3escore
)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
