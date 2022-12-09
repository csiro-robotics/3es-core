# Functions for configuring third eye scene targets.

# tes_configure_target_flags(TARGET)
#
# Configure compiler and linker flags for the given TARGET. This affects the warnings for the project and adds debug
# symbols to release builds.
#
# Flags are added as PRIVATE and do not propagate to downstream libraries.
function(tes_configure_target_flags TARGET)
  # Compiler configuration for 3es targets.
  target_compile_options(${TARGET} PRIVATE
    #-------------------------------------
    # Clang/GCC common settings
    #-------------------------------------
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>:-pedantic -Wall -Wextra -Wconversion -Werror=vla -Wno-parentheses -Wno-variadic-macros>
    # Enable debug symbols for release builds. These can be stripped if required for distribution.
    $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>,$<CONFIG:Release>>:-g>
    #-------------------------------------
    # Clang settings
    #-------------------------------------
    # Disable warning about and/or precedence. Remember when it comes to precedence:
    # - and is equivalent to multiplication
    # - or is equilvanet to addition
    # Now we don't need excessive brackets (parentheses for the Americans)
    $<$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>:-Wno-logical-op-parentheses>
    #-------------------------------------
    # MSC settings.
    #-------------------------------------
    # Correctly report the C++ version number.
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
    # Set warning level 3
    $<$<CXX_COMPILER_ID:MSVC>:/W3>
    # Enable warnings to match GCC behaviour on top of warning level 3.
    # The warning syntax is /wd?3\d\d\d\d which means at warning level 3 enable (or disable with /wd)
    # warning number xxxx
    $<$<CXX_COMPILER_ID:MSVC>:/w34100> # Unused arguments.
    # Enable PDB files (debug symbols) for release builds.
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/Zi>
  )

  # Target link options was added in CMake 3.13 (Ubuntu 18.04 apt provides 3.10, 20.04 is 3.16).
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
    # For Windows ensure linking provides debug symbols in release builds.
    target_link_options(${TARGET} PRIVATE
      $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/debug>
    )
  endif(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
endfunction(tes_configure_target_flags)

# tes_configure_target_properties(TARGET)
#
# Configure properties for the given TARGET
function(tes_configure_target_properties TARGET)
  # Propagate the debug suffix to executables.
  get_target_property(_target_type ${TARGET} TYPE)
  if(_target_type STREQUAL "EXECUTABLE")
    set_target_properties(${TARGET} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
  endif(_target_type STREQUAL "EXECUTABLE")
endfunction(tes_configure_target_properties)

# tes_configure_target_properties(TARGET)
#
# Configure installation of the given TARGET
function(tes_target_install TARGET)
  install(TARGETS ${TARGET} EXPORT 3es-config-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
  )

  get_target_property(_target_type ${TARGET} TYPE)
  # Install PDB files (MSVC) in part to avoid unsupressable linker warnings.
  if(_target_type STREQUAL "EXECUTABLE" OR _target_type STREQUAL "MODULE" OR _target_type STREQUAL "SHARED")
    install(FILES $<TARGET_PDB_FILE:${TARGET}> DESTINATION bin OPTIONAL)
  endif()
endfunction(tes_target_install)

# tes_configure_version(TARGET)
#
# Configure versioning of the given target.
function(tes_target_version TARGET)
  # Setup target versioning.
  set_target_properties(${TARGET} PROPERTIES
    # Build version.
    VERSION ${TES_VERSION}
    # API version.
    SOVERSION ${TES_VERSION_MAJOR} # API version.
    # Compatibility versioning. Referenced via the COMPATIBLE_INTERFACE_STRING property.
    INTERFACE_3escore_MAJOR_VERSION ${TES_VERSION_MAJOR}
  )

  # Help enforce version compability by ensuring all link dependencies expect the same INTERFACE_3escore_MAJOR_VERSION
  set_property(TARGET ${TARGET} APPEND PROPERTY
    COMPATIBLE_INTERFACE_STRING INTERFACE_3escore_MAJOR_VERSION
  )
endfunction(tes_target_version)

# tes_configure_target(TARGET [SKIP feature1 [... featureN]])
#
# Apply various configuration actions on the given TARGET. All actions are enabled by default, but can be disabled by
# using the SKIP feature arguments.
#
# Available features and their associated functions are:
#
# - FLAGS tes_configure_target_flags()
# - PROPERTIES tes_configure_target_properties()
# - INSTALL tes_target_install()
# - VERSION tes_target_version()
function(tes_configure_target TARGET)
  cmake_parse_arguments(ARG "" "" "SKIP")
  if(NOT "FLAGS" IN_LIST ARG_SKIP)
    tes_configure_target_flags(${TARGET})
  endif(NOT "FLAGS" IN_LIST ARG_SKIP)
  if(NOT "PROPERTIES" IN_LIST ARG_SKIP)
    tes_configure_target_properties(${TARGET})
  endif(NOT "PROPERTIES" IN_LIST ARG_SKIP)
  if(NOT "INSTALL" IN_LIST ARG_SKIP)
    tes_target_install(${TARGET})
  endif(NOT "INSTALL" IN_LIST ARG_SKIP)
  if(NOT "VERSION" IN_LIST ARG_SKIP)
    tes_target_version(${TARGET})
  endif(NOT "VERSION" IN_LIST ARG_SKIP)
endfunction(tes_configure_target)

# Configure a TES unit test target. This skips installation and versionining for the target.
function(tes_configure_unit_test_target TARGET)
  tes_configure_target(${TARGET} SKIP INSTALL VERSION)
  add_test(NAME ${TARGET} COMMAND ${TARGET} --gtest_output=xml:test-reports/)
endfunction(tes_configure_unit_test_target TARGET)

if(CMAKE_VERSION VERSION_LESS 3.13)
  # For Windows ensure linking provides debug symbols in release builds.
  if(MSVC)
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "/debug ${CMAKE_MODULE_LINKER_FLAGS_RELEASE}")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/debug ${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
  endif(MSVC)
endif(CMAKE_VERSION VERSION_LESS 3.13)

# Check for known compiler.
set(TES_KNOWN_COMPILERS 
  "AppleClang"
  "Clang"
  "GNU"
  "MSVC"
)

list(FIND TES_KNOWN_COMPILERS "${CMAKE_CXX_COMPILER_ID}" TES_KNOWN_COMPILER_IDX)

if(TES_KNOWN_COMPILER_IDX LESS 0)
  message("Unknown compiler ID: ${CMAKE_CXX_COMPILER_ID}. Additional compiler options may be required.")
endif(TES_KNOWN_COMPILER_IDX LESS 0)
