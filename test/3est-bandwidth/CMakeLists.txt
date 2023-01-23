add_executable(3est-bandwidth Bandwidth.cpp)
tes_configure_target(3est-bandwidth)
target_link_libraries(3est-bandwidth
  PRIVATE
    3escore
)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
