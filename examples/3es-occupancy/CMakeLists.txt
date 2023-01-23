# FIXME: this example is stale.
option(TES_WITH_OCTOMAP_EXAMPLE "Enable the octomap example visualisation if available?" Off)

if(NOT TES_WITH_OCTOMAP_EXAMPLE)
  return()
endif(NOT TES_WITH_OCTOMAP_EXAMPLE)

find_package(octomap CONFIG)

if(NOT octomap_FOUND)
  message(STATUS "Octomap not found. Skipping.")
  return()
endif(NOT octomap_FOUND)

# Handle older and newer octomap incldue dir variables.
# Convert older (1.8) to newer (currently in devel)
if(DEFINED OCTOMAP_INCLUDE_DIR AND NOT DEFINED OCTOMAP_INCLUDE_DIRS)
  set(OCTOMAP_INCLUDE_DIRS "${OCTOMAP_INCLUDE_DIR}")
endif(DEFINED OCTOMAP_INCLUDE_DIR AND NOT DEFINED OCTOMAP_INCLUDE_DIRS)

if(NOT DEFINED OCTOMAP_INCLUDE_DIRS)
  message(STATUS "Octomap not found. Skipping 3es-occupancy example.")
  return()
endif(NOT DEFINED OCTOMAP_INCLUDE_DIRS)

# Need to add boost link directories because of LIBLAS.
find_package(Boost COMPONENTS "thread")
link_directories("${Boost_LIBRARY_DIRS}")

list(APPEND SOURCES
  3es-occupancy.h
  3es-occupancy.cpp
  debugids.h
  occupancyloader.cpp
  occupancyloader.h
  occupancymesh.cpp
  occupancymesh.h
  p2p.h
  3rd-party/tinyply.h
)

add_executable(3es-occupancy ${SOURCES})
tes_configure_target(3es-occupancy)

target_link_libraries(3es-occupancy
  PRIVATE
    3escore
    octomap-static
    octomath-static
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
