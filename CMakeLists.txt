cmake_minimum_required(VERSION 3.21)

project(
  tuvx
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/cmake")

# Options
option(TUVX_ENABLE_TESTS "Enable test suite" ON)
option(TUVX_ENABLE_OPENMP "Enable OpenMP support" OFF)
option(TUVX_ENABLE_MPI "Enable MPI support" OFF)
option(TUVX_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)

# Cache variable for default vector size
set(TUVX_DEFAULT_VECTOR_SIZE 4 CACHE STRING "Default vector size for SIMD operations")

# Include dependencies
include(dependencies)

# Include static analyzers if enabled
if(TUVX_ENABLE_CLANG_TIDY)
  include(StaticAnalyzers)
endif()

# Add the library
add_subdirectory(src)

# Add tests if enabled
if(TUVX_ENABLE_TESTS)
  enable_testing()
  include(test_util)
  add_subdirectory(test)
endif()
