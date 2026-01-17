# TUV-x C++ Project Instructions

## Project Context

TUV-x C++ is a modern C++20 translation of the TUV-x (Tropospheric Ultraviolet-Visible) radiative transfer model. This is part of the MUSICA ecosystem and follows MICM's coding patterns.

**Reference project**: `/Users/fillmore/EarthSystem/MICM` - consult this for coding style questions.

## Build Commands

```bash
# Configure (from project root)
mkdir build && cd build
cmake .. -DTUVX_ENABLE_TESTS=ON

# Build
make -j4

# Run tests
ctest --output-on-failure

# Build with options
cmake .. -DTUVX_ENABLE_OPENMP=ON -DTUVX_ENABLE_CLANG_TIDY=ON
```

## Coding Standards

### Naming Conventions
- **Functions/Methods**: `CamelCase` (e.g., `CalculateOpticalDepth`)
- **Classes**: `CamelCase` (e.g., `WavelengthGrid`)
- **Member variables**: `snake_case_` with trailing underscore (e.g., `grid_points_`)
- **Constants**: `kCamelCase` prefix (e.g., `kBoltzmannConstant`)
- **Macros**: `TUVX_UPPER_CASE` (e.g., `TUVX_INTERNAL_ERROR`)
- **Namespaces**: lowercase (e.g., `tuvx::constants`)

### File Organization
- Headers in `include/tuvx/`
- Implementation (if needed) in `src/`
- Tests in `test/unit/` mirroring include structure
- One class per header file typically

### Header Style
```cpp
#pragma once

#include <system_headers>

#include <tuvx/project_headers.hpp>

namespace tuvx
{
  // Allman brace style
  class Example
  {
   public:
    void DoSomething();

   private:
    int value_;
  };
}  // namespace tuvx
```

### Error Handling
- Use `TUVX_INTERNAL_ERROR("message")` for unexpected internal states
- Error codes defined in `include/tuvx/util/error.hpp`
- Exceptions inherit from `std::runtime_error`
- Include file/line info in error messages

### Documentation
- Use `///` for Doxygen-style documentation
- Document `@brief`, `@param`, `@return` for public APIs
- Keep comments concise; code should be self-documenting

## Testing Patterns

### Creating Tests
```cpp
#include <tuvx/component.hpp>
#include <gtest/gtest.h>

TEST(ComponentTest, DescriptiveName)
{
  // Arrange
  auto component = CreateComponent();

  // Act
  auto result = component.Calculate();

  // Assert
  EXPECT_NEAR(result, expected, tolerance);
}
```

### Adding New Test Files
1. Create test file in `test/unit/<category>/test_<name>.cpp`
2. Add to `test/unit/CMakeLists.txt`:
   ```cmake
   create_tuvx_test(test_name category/test_name.cpp)
   ```

## Key Patterns

### Physical Constants
Use constants from `tuvx::constants` namespace - never hardcode physical values.

### Array Utilities
Use `tuvx::array::` functions for common operations:
- `Linspace`, `Logspace` for grid generation
- `MergeSorted` for combining wavelength grids
- `AlmostEqual` for floating-point comparison
- `FindString` for case-insensitive string lookup

### Header-Only Design
This is a header-only library. Implementation goes in headers:
```cpp
// In header file
inline double Calculate(double x)
{
  return x * 2.0;
}
```

## Common Tasks

### Adding a New Utility
1. Create header in `include/tuvx/util/`
2. Add include to `include/tuvx/tuvx.hpp`
3. Create test in `test/unit/util/`
4. Update `test/unit/CMakeLists.txt`

### Adding a New Component (Future)
1. Create header in `include/tuvx/<component>/`
2. Create tests in `test/unit/<component>/`
3. Update ARCHITECTURE.md with component description

## Formatting

Run clang-format before committing:
```bash
find include src test -name "*.hpp" -o -name "*.cpp" | xargs clang-format -i
```

## Dependencies

- C++20 features used: `<concepts>`, `<ranges>`, `std::optional`, designated initializers
- Google Test for testing
- Optional: OpenMP, MPI

## Related Projects

- **MICM**: `/Users/fillmore/EarthSystem/MICM` - Reference for coding patterns
- **TUV-x Fortran**: `/Users/fillmore/EarthSystem/TUV-x` - Original implementation to translate

## Phase Status

- [x] Phase 1: Foundation (utilities, error handling, constants)
- [x] Phase 2: Data structures (grids, profiles, interpolation)
- [ ] Phase 3: Radiative transfer components
- [ ] Phase 4+: Solver implementation
