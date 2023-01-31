
add_subdirectory(3esSphereView)
set_target_properties(3esSphereView PROPERTIES FOLDER examples)

add_subdirectory(3esOccupancy)
if(TARGET 3esOccupancy)
  set_target_properties(3esOccupancy PROPERTIES FOLDER examples)
endif(TARGET 3esOccupancy)
