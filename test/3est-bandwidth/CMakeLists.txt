add_executable(3estBandwidth Bandwidth.cpp)
tes_configure_target(3estBandwidth)
target_link_libraries(3estBandwidth
  PRIVATE
    3escore
)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
