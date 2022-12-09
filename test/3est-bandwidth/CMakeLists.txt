add_executable(3est-bandwidth 3est-bandwidth.cpp)
tes_configure_target(3est-bandwidth)
target_link_libraries(3est-bandwidth
  PRIVATE
    3es-core
)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
