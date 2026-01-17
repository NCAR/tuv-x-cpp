---
description: Help translate Fortran TUV-x code to C++
args: "<fortran-module-or-file>"
---

# Translate Fortran to C++

Assist with translating TUV-x Fortran code to modern C++.

## Process

1. **Locate the Fortran source**:
   - Search in `/Users/fillmore/EarthSystem/TUV-x/src/` for the specified module or file
   - Read and understand the Fortran implementation

2. **Study MICM patterns**:
   - Look at equivalent patterns in `/Users/fillmore/EarthSystem/MICM/include/micm/`
   - Note naming conventions, error handling, and structure

3. **Plan the translation**:
   - Map Fortran types to C++ equivalents
   - Identify which TUV-x utilities to use (constants, array functions, etc.)
   - Design the class/function interface

4. **Create C++ implementation**:
   - Header file in `include/tuvx/<category>/`
   - Follow CLAUDE.md coding standards
   - Use `tuvx::` namespace

5. **Create unit tests**:
   - Test file in `test/unit/<category>/`
   - Port any existing Fortran tests
   - Add edge case coverage

6. **Update build system**:
   - Add test to `test/unit/CMakeLists.txt`

## Fortran to C++ Type Mapping

| Fortran | C++ |
|---------|-----|
| `real(dk)` | `double` |
| `integer` | `int` or `std::size_t` |
| `character(len=*)` | `std::string` or `std::string_view` |
| `logical` | `bool` |
| `real(dk), dimension(:)` | `std::vector<double>` |
| `real(dk), dimension(:,:)` | `std::vector<std::vector<double>>` or custom Matrix |
| `type` | `class` or `struct` |
| `module` | `namespace` + header file |

## Examples

- `/translate grid` - Translate grid module
- `/translate cross_section` - Translate cross section handling
- `/translate src/radiator.F90` - Translate specific file
