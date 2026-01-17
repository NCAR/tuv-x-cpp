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
├── ARCHITECTURE.md             # This file
├── CLAUDE.md                   # Project instructions for Claude Code
├── NUMERICAL-TESTS.md          # Numerical validation test ideas
├── .clang-format               # Code formatting (Google style, Allman braces)
├── .clang-tidy                 # Static analysis rules
├── cmake/
│   ├── dependencies.cmake      # External dependencies (GTest, OpenMP, MPI)
│   ├── StaticAnalyzers.cmake   # Clang-tidy integration
│   └── test_util.cmake         # Test helper macros
├── include/tuvx/
│   ├── tuvx.hpp                # Main convenience header
│   ├── version.hpp             # Generated version info
│   ├── util/                   # Utilities (Phase 1)
│   │   ├── constants.hpp       # Physical constants (SI, CODATA 2019)
│   │   ├── error.hpp           # Error code definitions
│   │   ├── internal_error.hpp  # Exception handling
│   │   └── array.hpp           # Array utilities
│   ├── grid/                   # Grid system (Phase 2)
│   │   ├── grid_spec.hpp       # Grid specification
│   │   ├── grid.hpp            # Immutable grid
│   │   ├── mutable_grid.hpp    # Host-updatable grid
│   │   └── grid_warehouse.hpp  # Grid collection manager
│   ├── profile/                # Profile containers (Phase 2)
│   │   ├── profile_spec.hpp    # Profile specification
│   │   ├── profile.hpp         # Immutable profile
│   │   ├── mutable_profile.hpp # Host-updatable profile
│   │   └── profile_warehouse.hpp
│   ├── interpolation/          # Interpolation (Phase 2)
│   │   ├── interpolator.hpp    # Interpolator concept
│   │   ├── linear_interpolator.hpp
│   │   └── conserving_interpolator.hpp
│   ├── cross_section/          # Cross-sections (Phase 3A)
│   │   ├── cross_section.hpp   # Base interface
│   │   ├── cross_section_warehouse.hpp
│   │   └── types/
│   │       ├── base.hpp        # Temperature-independent
│   │       └── o3.hpp          # O3 with T-dependence
│   ├── quantum_yield/          # Quantum yields (Phase 3A)
│   │   ├── quantum_yield.hpp   # Base interface
│   │   ├── quantum_yield_warehouse.hpp
│   │   └── types/
│   │       ├── base.hpp        # Constant quantum yield
│   │       └── o3_o1d.hpp      # O3->O1D T-dependent
│   ├── radiator/               # Radiators (Phase 3B)
│   │   ├── radiator.hpp        # Base interface
│   │   ├── radiator_state.hpp  # Optical properties
│   │   ├── radiator_warehouse.hpp
│   │   └── types/
│   │       └── from_cross_section.hpp
│   ├── radiation_field/        # Radiation field (Phase 3B)
│   │   └── radiation_field.hpp
│   ├── solar/                  # Solar components (Phase 3C)
│   │   ├── solar_position.hpp  # Solar geometry
│   │   └── extraterrestrial_flux.hpp
│   ├── surface/                # Surface properties (Phase 3C)
│   │   └── surface_albedo.hpp
│   ├── spherical_geometry/     # Spherical corrections (Phase 3C)
│   │   └── spherical_geometry.hpp
│   ├── solver/                 # RT solvers (Phase 3D)
│   │   ├── solver.hpp          # Solver interface
│   │   └── delta_eddington.hpp # Two-stream solver
│   ├── photolysis/             # Photolysis rates (Phase 3D)
│   │   └── photolysis_rate.hpp
│   └── model/                  # Model orchestration (Phase 4)
│       ├── model_config.hpp    # Configuration
│       ├── model_output.hpp    # Output container
│       └── tuv_model.hpp       # Main TuvModel class
├── src/
│   ├── CMakeLists.txt          # Library target definition
│   └── version.hpp.in          # Version template
└── test/
    ├── CMakeLists.txt
    └── unit/
        ├── util/               # Utility tests
        ├── grid/               # Grid tests
        ├── profile/            # Profile tests
        ├── interpolation/      # Interpolation tests
        ├── cross_section/      # Cross-section tests
        ├── quantum_yield/      # Quantum yield tests
        ├── radiator/           # Radiator tests
        ├── radiation_field/    # Radiation field tests
        ├── solar/              # Solar tests
        ├── surface/            # Surface tests
        ├── spherical_geometry/ # Spherical geometry tests
        ├── solver/             # Solver tests
        ├── photolysis/         # Photolysis tests
        └── model/              # Model tests
```

## Component Architecture

### Complete System (Current State)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              TuvModel (Phase 4)                              │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │  ModelConfig                    ModelOutput                             │ │
│  │  - Wavelength/altitude grids    - RadiationField                       │ │
│  │  - Solar parameters             - Photolysis rates (J-values)          │ │
│  │  - Atmospheric profiles         - Metadata (SZA, day, etc.)            │ │
│  │  - Surface properties           - Accessor methods                     │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
│                                      │                                       │
│  ┌───────────────────────────────────┴────────────────────────────────────┐ │
│  │                     Radiative Transfer (Phase 3D)                       │ │
│  │  ┌──────────────────────┐  ┌─────────────────────────────────────────┐ │ │
│  │  │   DeltaEddington     │  │         PhotolysisRateSet               │ │ │
│  │  │   - Two-stream RT    │  │  - J = ∫ F(λ) × σ(λ) × φ(λ) dλ         │ │ │
│  │  │   - Direct/diffuse   │  │  - Multiple reactions                   │ │ │
│  │  │   - Surface reflect  │  │  - Altitude profiles                    │ │ │
│  │  └──────────────────────┘  └─────────────────────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
│                                      │                                       │
│  ┌───────────────────────────────────┴────────────────────────────────────┐ │
│  │                    Supporting Components (Phase 3B-C)                   │ │
│  │  ┌─────────────┐  ┌──────────────┐  ┌────────────────┐  ┌───────────┐ │ │
│  │  │  Radiators  │  │  SolarFlux   │  │  Spherical     │  │  Surface  │ │ │
│  │  │  - Optical  │  │  - ASTM E490 │  │  Geometry      │  │  Albedo   │ │ │
│  │  │    depth    │  │  - Position  │  │  - Slant paths │  │  - λ-dep  │ │ │
│  │  │  - SSA, g   │  │  - Day/time  │  │  - Chapman fn  │  │  - Types  │ │ │
│  │  └─────────────┘  └──────────────┘  └────────────────┘  └───────────┘ │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
│                                      │                                       │
│  ┌───────────────────────────────────┴────────────────────────────────────┐ │
│  │                  Cross-Sections & Quantum Yields (Phase 3A)             │ │
│  │  ┌──────────────────────────────┐  ┌─────────────────────────────────┐ │ │
│  │  │       CrossSection           │  │        QuantumYield             │ │ │
│  │  │  - BaseCrossSection          │  │  - ConstantQuantumYield         │ │ │
│  │  │  - O3CrossSection (T-dep)    │  │  - O3O1DQuantumYield (T-dep)    │ │ │
│  │  │  - CrossSectionWarehouse     │  │  - QuantumYieldWarehouse        │ │ │
│  │  └──────────────────────────────┘  └─────────────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
│                                      │                                       │
│  ┌───────────────────────────────────┴────────────────────────────────────┐ │
│  │                     Data Structures (Phase 2)                           │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌─────────────────────────────┐  │ │
│  │  │    Grids     │  │   Profiles   │  │      Interpolation          │  │ │
│  │  │  - Wavelength│  │  - T, p, ρ   │  │  - Linear                   │  │ │
│  │  │  - Altitude  │  │  - O3, O2    │  │  - Area-conserving          │  │ │
│  │  │  - Warehouse │  │  - Warehouse │  │  - Interpolator concept     │  │ │
│  │  └──────────────┘  └──────────────┘  └─────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
│                                      │                                       │
│  ┌───────────────────────────────────┴────────────────────────────────────┐ │
│  │                        Utilities (Phase 1)                              │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────────────┐ │ │
│  │  │  Constants  │  │   Errors    │  │       Array Utilities           │ │ │
│  │  │  - Physical │  │  - Codes    │  │  - FindString, AlmostEqual      │ │ │
│  │  │  - Math     │  │  - Categor. │  │  - Linspace, Logspace           │ │ │
│  │  │  - Atmos.   │  │  - Except.  │  │  - MergeSorted, Interpolate     │ │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘
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
| 1 | **Complete** | Foundation, utilities, error handling, constants |
| 2 | **Complete** | Grid system, profile containers, interpolation |
| 3A | **Complete** | Cross-sections, quantum yields, warehouses |
| 3B | **Complete** | Radiators, radiator state, radiation field |
| 3C | **Complete** | Solar position, surface albedo, spherical geometry |
| 3D | **Complete** | Delta-Eddington solver, photolysis rate calculator |
| 4 | **Complete** | Model orchestration (TuvModel, ModelConfig, ModelOutput) |
| 5 | Planned | Numerical validation against TUV-x Fortran |
| 6 | Planned | Additional cross-section/quantum yield types |
| 7 | Planned | C/Fortran interfaces |
| 8 | Planned | Performance optimization, SIMD/OpenMP |

## Current Test Coverage

**493 tests passing** across all components:
- Utility tests (constants, errors, arrays)
- Grid tests (construction, operations, warehouse)
- Profile tests (construction, operations, warehouse)
- Interpolation tests (linear, area-conserving)
- Cross-section tests (base, O3, warehouse)
- Quantum yield tests (constant, O3->O1D, warehouse)
- Radiator tests (state, radiator, warehouse)
- Radiation field tests
- Solar tests (position, extraterrestrial flux)
- Surface tests (albedo)
- Spherical geometry tests
- Solver tests (Delta-Eddington)
- Photolysis tests
- Model tests (TuvModel, scenarios)

## Key Implementation Notes

### Current Limitations (Empty Atmosphere)
The current model calculates radiation transfer through an "empty atmosphere" (no radiators configured by default). This means:
- Actinic flux is approximately constant with altitude
- No SZA dependence in attenuation (only cosine factor in irradiance)
- J-values are approximately constant with altitude

These behaviors are expected and documented in tests. Full physical behavior requires configuring radiators (O3, O2, aerosols, etc.).

### Standard Atmosphere Support
The `StandardAtmosphere` namespace provides US Standard Atmosphere 1976 profiles:
- `Temperature(altitude_km)` - Temperature profile
- `Pressure(altitude_km)` - Pressure profile
- `AirDensity(T, P)` - Air density calculation
- `GenerateTemperatureProfile()`, `GeneratePressureProfile()`, etc.

### Photolysis Rate Calculation
J-values are computed as:
```
J = ∫ F(λ) × σ(λ,T) × φ(λ,T) dλ
```
Where:
- F(λ) = Actinic flux [photons/cm²/s/nm]
- σ(λ,T) = Absorption cross-section [cm²]
- φ(λ,T) = Quantum yield [dimensionless]

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

### Numerical Validation (Planned)
See `NUMERICAL-TESTS.md` for detailed validation test plans:
- Delta-Eddington analytical benchmarks
- Toon et al. (1989) reference cases
- TUV-x Fortran parity tests
- Published J-value comparisons

## References

- [MICM](https://github.com/NCAR/micm) - Model Independent Chemistry Module (reference implementation)
- [TUV-x](https://github.com/NCAR/tuv-x) - Original Fortran implementation
- [MUSICA](https://github.com/NCAR/musica) - Multi-Scale Infrastructure for Chemistry and Aerosols
- Toon, O.B., et al., 1989, J. Geophys. Res., 94, 16287-16301 (Two-stream methods)
- Madronich, S., 1987, J. Geophys. Res., 92, 9740-9752 (Photolysis rates)
