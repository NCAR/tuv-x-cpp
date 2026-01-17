# Helper function to create a test executable
function(create_tuvx_test test_name test_source)
  add_executable(${test_name} ${test_source})
  target_link_libraries(${test_name}
    PRIVATE
      musica::tuvx
      GTest::gtest_main
  )

  # Add OpenMP if enabled
  if(TUVX_ENABLE_OPENMP)
    target_link_libraries(${test_name} PRIVATE OpenMP::OpenMP_CXX)
  endif()

  # Add MPI if enabled
  if(TUVX_ENABLE_MPI)
    target_link_libraries(${test_name} PRIVATE MPI::MPI_CXX)
  endif()

  include(GoogleTest)
  gtest_discover_tests(${test_name})
endfunction()
