find_package(GTest QUIET)

add_subdirectory(3est-bandwidth)
add_subdirectory(3est-primitive-server)
add_subdirectory(3est-server)
add_subdirectory(3est-tessellate)

set_target_properties(3est-bandwidth PROPERTIES FOLDER test)
set_target_properties(3est-primitive-server PROPERTIES FOLDER test)
set_target_properties(3est-server PROPERTIES FOLDER test)
set_target_properties(3est-tessellate PROPERTIES FOLDER test)

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
