# Project setup and minimum cmake version.
cmake_minimum_required(VERSION 3.5)

# Setup project details.
project(3es)
include(3es-core/cmake/3es-version.cmake)
set(ConfigPackageLocation lib/cmake/3es)

# C++ standards setup.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
# Ensure -fPIC is added.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add cmake directory to the modules path.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH}")

# Ensure debug libraries are built with different named to release builds. This is to address issues such as MSVC
# having different debug and release runtime libraries. For a well setup API, one which hides resource allocation and
# ensures symmetrical deallcation occurs from the same allocator, this won't be a problem, but the consistency is
# useful.
set(CMAKE_DEBUG_POSTFIX "d")
# Marshall all binaries to the same directory. This is expecially useful on Windows when trying to run exectuables from
# this project with shared libraries. Otherwise those shared libraries aren't on the path. Note that other package
# binaries should be on the path already.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Add project level options here.
option(TES_BUILD_EXAMPLES "Build examples?" OFF)
option(TES_BUILD_TESTS "Build unit tests?" OFF)

# Allow the use of folders to group targets in supporting environments.
# For example Visual Studio solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Manage warnings, disabling ones we want to ignore.
include(compilerSetup)

# Include sub projects.
add_subdirectory(3es-core)
if(TES_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif(TES_BUILD_EXAMPLES)

add_subdirectory(utils)

if(TES_BUILD_TESTS)
  # We can enable testing here and/or in the subdirectory, but doing it here allows us to run CTest from the build root.
  # To run the tests, we execute:
  #   CTest -C [Debug|Release|RelWithDebInfo|MinSizeRel] --output-on-failure
  # CTest normally shows only a very terse test ouput, but we make sure failed tests show all output by adding
  #   --output-on-failure
  # The full test output is always available in:
  #   <build>/Testing/Temporary/LastTest.log
  # Note: not all projects under tests actually add something for CTest.
  enable_testing()
  add_subdirectory(test)
endif(TES_BUILD_TESTS)
