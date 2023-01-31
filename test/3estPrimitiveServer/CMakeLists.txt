add_executable(3estPrimitiveServer PrimitiveServer.cpp)
tes_configure_target(3estPrimitiveServer)
target_link_libraries(3estPrimitiveServer
  PRIVATE
    3escore
)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
