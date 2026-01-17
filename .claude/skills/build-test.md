---
description: Build the project and run all unit tests
---

# Build and Test TUV-x

Execute the following steps:

1. Change to project root: `/Users/fillmore/EarthSystem/TUV-x-cpp`

2. Check if build directory exists and is configured:
   - If `build/` doesn't exist or `build/Makefile` doesn't exist:
     ```bash
     mkdir -p build && cd build && cmake .. -DTUVX_ENABLE_TESTS=ON
     ```
   - Otherwise just `cd build`

3. Build the project:
   ```bash
   make -j4
   ```

4. Run all tests:
   ```bash
   ctest --output-on-failure
   ```

5. Report a summary:
   - Number of tests passed/failed
   - If any failures, show the failing test names and brief error descriptions
   - Build warnings if any
