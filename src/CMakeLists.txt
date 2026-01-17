# Configure version header
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in
  ${CMAKE_BINARY_DIR}/include/tuvx/version.hpp
  @ONLY
)

# Create header-only interface library
add_library(tuvx INTERFACE)
add_library(musica::tuvx ALIAS tuvx)

target_include_directories(tuvx
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(tuvx INTERFACE cxx_std_20)

# Add OpenMP if enabled
if(TUVX_ENABLE_OPENMP)
  target_link_libraries(tuvx INTERFACE OpenMP::OpenMP_CXX)
endif()

# Add MPI if enabled
if(TUVX_ENABLE_MPI)
  target_link_libraries(tuvx INTERFACE MPI::MPI_CXX)
endif()

# Pass default vector size as compile definition
target_compile_definitions(tuvx
  INTERFACE
    TUVX_DEFAULT_VECTOR_SIZE=${TUVX_DEFAULT_VECTOR_SIZE}
)
