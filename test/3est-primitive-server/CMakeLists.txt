add_executable(3est-primitive-server PrimitiveServer.cpp)
tes_configure_target(3est-primitive-server)
target_link_libraries(3est-primitive-server
  PRIVATE
    3escore
)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
