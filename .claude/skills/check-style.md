---
description: Verify code follows MICM/TUV-x coding patterns
args: "<file-path>"
---

# Check Code Style

Verify that code follows MICM patterns and TUV-x conventions.

## Checks to Perform

1. **Read the specified file**

2. **Naming conventions**:
   - Functions/methods: `CamelCase` (e.g., `CalculateFlux`)
   - Classes: `CamelCase` (e.g., `WavelengthGrid`)
   - Member variables: `snake_case_` with trailing underscore
   - Constants: `kCamelCase` (e.g., `kSpeedOfLight`)
   - Macros: `TUVX_UPPER_CASE`
   - Namespaces: `lowercase`

3. **Brace style**:
   - Allman style (braces on their own line)
   - Correct for classes, functions, control structures

4. **Header structure**:
   - `#pragma once` at top
   - System includes first, then project includes
   - Proper namespace wrapping

5. **Documentation**:
   - Public APIs have `///` documentation
   - `@brief`, `@param`, `@return` where appropriate

6. **Error handling**:
   - Uses `TUVX_INTERNAL_ERROR` for internal errors
   - Doesn't throw raw strings

7. **Compare with MICM** (if similar component exists):
   - Check `/Users/fillmore/EarthSystem/MICM/include/micm/` for patterns

## Report Format

List any deviations found with:
- Line number
- Issue description
- Suggested fix

If no issues found, confirm the file follows conventions.
