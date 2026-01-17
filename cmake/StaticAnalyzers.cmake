# Clang-tidy setup
find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if(NOT CLANG_TIDY_EXE)
  message(WARNING "clang-tidy not found, static analysis will be disabled")
else()
  message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
  set(CMAKE_CXX_CLANG_TIDY
    ${CLANG_TIDY_EXE}
    -p=${CMAKE_BINARY_DIR}
  )
endif()
