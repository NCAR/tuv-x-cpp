---
description: Scaffold a new TUV-x component (header + tests)
args: "<category> <component-name>"
---

# Create New Component

Scaffold a new TUV-x component with header and test files.

## Arguments

- `category`: Component category (e.g., `grid`, `profile`, `radiator`, `solver`)
- `component-name`: Name of the component in CamelCase (e.g., `WavelengthGrid`)

## Steps

1. **Create header file**: `include/tuvx/<category>/<component_name>.hpp`

   Template:
   ```cpp
   #pragma once

   #include <vector>

   namespace tuvx
   {
     /// @brief Brief description of ComponentName
     class ComponentName
     {
      public:
       /// @brief Constructor description
       ComponentName();

       /// @brief Main method description
       /// @param param Description
       /// @return Description
       void DoSomething();

      private:
       // Member variables with trailing underscore
     };
   }  // namespace tuvx
   ```

2. **Create test file**: `test/unit/<category>/test_<component_name>.cpp`

   Template:
   ```cpp
   #include <tuvx/<category>/<component_name>.hpp>

   #include <gtest/gtest.h>

   TEST(ComponentNameTest, Construction)
   {
     tuvx::ComponentName component;
     // Test construction
   }

   TEST(ComponentNameTest, BasicOperation)
   {
     // Test basic functionality
   }
   ```

3. **Update test CMakeLists.txt**: `test/unit/CMakeLists.txt`

   Add:
   ```cmake
   create_tuvx_test(test_<component_name> <category>/test_<component_name>.cpp)
   ```

4. **Create category directory if needed**:
   - `mkdir -p include/tuvx/<category>`
   - `mkdir -p test/unit/<category>`

5. **Report created files** and next steps

## Examples

- `/new-component grid WavelengthGrid`
- `/new-component profile AtmosphericProfile`
- `/new-component radiator Aerosol`
