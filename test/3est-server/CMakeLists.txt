
add_executable(3est-server ServerTest.cpp)
tes_configure_target(3est-server)
target_link_libraries(3est-server
  PRIVATE
    3escore
)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
