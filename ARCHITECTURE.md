# TUV-x C++ Architecture

## Overview

TUV-x C++ is a modern C++20 translation of the TUV-x (Tropospheric Ultraviolet-Visible) radiative transfer model. The project follows MICM's coding patterns and project structure to ensure consistency within the MUSICA ecosystem.

## Design Philosophy

### Header-Only Library
TUV-x is implemented as a header-only library, providing:
- Zero link-time dependencies for consumers
- Template flexibility for different numeric types
- Simplified integration into other projects
- Compile-time optimization opportunities

### MICM Compatibility
The codebase mirrors MICM patterns for:
- Namespace structure (`tuvx::`)
- Error handling (`std::error_code` + custom categories)
- Naming conventions (CamelCase functions, `k` prefix for constants)
- CMake organization and target naming (`musica::tuvx`)

## Project Structure

```
TUV-x-cpp/
├── CMakeLists.txt              # Root build configuration
├── .clang-format               # Code formatting (Google style, Allman braces)
├── .clang-tidy                 # Static analysis rules
├── cmake/
│   ├── dependencies.cmake      # External dependencies (GTest, OpenMP, MPI)
│   ├── StaticAnalyzers.cmake   # Clang-tidy integration
│   └── test_util.cmake         # Test helper macros
├── include/tuvx/
│   ├── tuvx.hpp                # Main convenience header
│   ├── version.hpp             # Generated version info
│   └── util/
│       ├── constants.hpp       # Physical constants (SI, CODATA 2019)
│       ├── error.hpp           # Error code definitions
│       ├── internal_error.hpp  # Exception handling
│       └── array.hpp           # Array utilities
├── src/
│   ├── CMakeLists.txt          # Library target definition
│   └── version.hpp.in          # Version template
└── test/
    ├── CMakeLists.txt
    └── unit/
        └── util/               # Unit tests for utilities
```

## Component Architecture

### Core Components (Planned)

```
┌─────────────────────────────────────────────────────────────────┐
│                        TUV-x Solver                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │   Grids     │  │  Profiles   │  │    Radiative Transfer   │  │
│  │ - Wavelength│  │ - O3        │  │  - Optical depths       │  │
│  │ - Altitude  │  │ - O2        │  │  - Actinic flux         │  │
│  │ - Time      │  │ - Temperature│ │  - Photolysis rates     │  │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘  │
│         │                │                      │                │
│         └────────────────┴──────────────────────┘                │
│                          │                                       │
│  ┌───────────────────────┴───────────────────────────────────┐  │
│  │                    Cross Sections & Quantum Yields         │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐ │  │
│  │  │   Absorbers  │  │   Radiators  │  │  Quantum Yields  │ │  │
│  │  │  (O3, O2,    │  │  (Aerosols,  │  │  (Temperature    │ │  │
│  │  │   NO2, etc.) │  │   Clouds)    │  │   dependent)     │ │  │
│  │  └──────────────┘  └──────────────┘  └──────────────────┘ │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

### Utility Layer (Phase 1 - Complete)

```
┌─────────────────────────────────────────────────────────────────┐
│                         Utilities                                │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │  Constants  │  │   Errors    │  │    Array Utilities      │  │
│  │ - Physical  │  │ - Codes     │  │  - FindString           │  │
│  │ - Math      │  │ - Categories│  │  - Linspace/Logspace    │  │
│  │ - Atmos.    │  │ - Exceptions│  │  - MergeSorted          │  │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Error Handling

### Error Categories
Errors are organized into domain-specific categories:
- `TUVX Configuration` - JSON/YAML parsing, missing keys
- `TUVX Grid` - Invalid bounds, size mismatches
- `TUVX Profile` - Invalid atmospheric profiles
- `TUVX Radiator` - Cross-section data errors
- `TUVX Internal Error` - Unexpected internal states

### Exception Pattern
```cpp
// Throwing internal errors with file/line tracking
TUVX_INTERNAL_ERROR("Unexpected null pointer in solver");

// Catching and handling
try {
    solver.Calculate();
} catch (const tuvx::TuvxInternalException& e) {
    std::cerr << e.what() << std::endl;  // Includes file:line
    std::cerr << "Error code: " << e.code() << std::endl;
}
```

## Build System

### CMake Targets
- `tuvx` - Header-only INTERFACE library
- `musica::tuvx` - Alias for namespace consistency
- `test_*` - Unit test executables

### Build Options
| Option | Default | Description |
|--------|---------|-------------|
| `TUVX_ENABLE_TESTS` | ON | Build unit tests |
| `TUVX_ENABLE_OPENMP` | OFF | Enable OpenMP parallelization |
| `TUVX_ENABLE_MPI` | OFF | Enable MPI support |
| `TUVX_ENABLE_CLANG_TIDY` | OFF | Enable static analysis |
| `TUVX_DEFAULT_VECTOR_SIZE` | 4 | Default SIMD vector width |

### Dependencies
- **Required**: C++20 compiler, CMake 3.21+
- **Testing**: Google Test (fetched automatically)
- **Optional**: OpenMP, MPI

## Phase Roadmap

| Phase | Status | Components |
|-------|--------|------------|
| 1 | Complete | Foundation, utilities, error handling |
| 2 | Planned | Grid system, profile containers |
| 3 | Planned | Cross-section database, interpolation |
| 4 | Planned | Radiative transfer solver |
| 5 | Planned | Photolysis rate calculations |
| 6 | Planned | C/Fortran interfaces |

## Performance Considerations

### Vectorization
- SIMD-friendly data layouts planned for hot paths
- `TUVX_DEFAULT_VECTOR_SIZE` controls default vectorization width
- Profile and grid data aligned for cache efficiency

### Parallelization Strategy
- **OpenMP**: Loop-level parallelism for wavelength/altitude iterations
- **MPI**: Domain decomposition for large-scale atmospheric models
- Thread-safe const interfaces for read-only operations

## Testing Strategy

### Unit Tests
- Located in `test/unit/`
- Use Google Test framework
- Cover edge cases, boundary conditions, numerical precision

### Integration Tests (Planned)
- Compare against Fortran TUV-x reference implementation
- Validate photolysis rates within acceptable tolerances

## References

- [MICM](https://github.com/NCAR/micm) - Model Independent Chemistry Module (reference implementation)
- [TUV-x](https://github.com/NCAR/tuv-x) - Original Fortran implementation
- [MUSICA](https://github.com/NCAR/musica) - Multi-Scale Infrastructure for Chemistry and Aerosols
