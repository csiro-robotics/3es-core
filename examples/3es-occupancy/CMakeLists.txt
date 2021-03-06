find_package(octomap QUIET)

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
set_target_properties(3es-occupancy PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

target_link_libraries(3es-occupancy 3es-core)

# Octomap is not well set up for MSVC builds. Static and shared libraries, for example, build to the same .lib file.
# Debug libraries aren't correctly defined or distinguished by default.
if(TARGET octomap)
  target_link_libraries(3es-occupancy octomap octomath)
else(TARGET octomap)
  target_link_libraries(3es-occupancy ${OCTOMAP_LIBRARIES})
endif(TARGET octomap)

target_include_directories(3es-occupancy
  PRIVATE
    $<TARGET_PROPERTY:3es-core,INCLUDE_DIRECTORIES>
    "${OCTOMAP_INCLUDE_DIRS}"
)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
