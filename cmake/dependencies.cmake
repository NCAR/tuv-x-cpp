include(FetchContent)

# Google Test
if(TUVX_ENABLE_TESTS)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

# OpenMP
if(TUVX_ENABLE_OPENMP)
  find_package(OpenMP REQUIRED)
endif()

# MPI
if(TUVX_ENABLE_MPI)
  find_package(MPI REQUIRED)
endif()
