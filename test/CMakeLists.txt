find_package(GTest QUIET)

add_subdirectory(3estBandwidth)
add_subdirectory(3estPrimitiveServer)
add_subdirectory(3estServer)
add_subdirectory(3estTessellate)

set_target_properties(3estBandwidth PROPERTIES FOLDER test)
set_target_properties(3estPrimitiveServer PROPERTIES FOLDER test)
set_target_properties(3estServer PROPERTIES FOLDER test)
set_target_properties(3estTessellate PROPERTIES FOLDER test)

# Add unit tests
if(GTEST_FOUND)
  add_subdirectory(3est-unit)
  set_target_properties(3est-unit PROPERTIES FOLDER test)
else(GTEST_FOUND)
  message("Missing GTest. Unit tests will not be built.")
endif(GTEST_FOUND)

if(TES_BUILD_VIEWER)
  # add_subdirectory(3est-viewer)
endif(TES_BUILD_VIEWER)
