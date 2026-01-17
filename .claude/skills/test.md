---
description: Run unit tests (optionally filtered by pattern)
args: "[test-pattern]"
---

# Run Unit Tests

Execute unit tests for TUV-x.

1. Change to build directory: `/Users/fillmore/EarthSystem/TUV-x-cpp/build`

2. If a test pattern argument is provided, run filtered tests:
   ```bash
   ctest --output-on-failure -R <test-pattern>
   ```

   If no pattern provided, run all tests:
   ```bash
   ctest --output-on-failure
   ```

3. Report results:
   - Total tests run
   - Pass/fail count
   - For failures: show test name and failure output

## Examples

- `/test` - Run all tests
- `/test Constants` - Run only ConstantsTest.* tests
- `/test Array` - Run only array-related tests
- `/test Linspace` - Run specific test by name
