# TUV-x C++

A header-only C++20 radiative transfer library for UV-Visible photolysis calculations.

Part of the [MUSICA](https://github.com/NCAR/musica) ecosystem.

## Overview

TUV-x C++ computes actinic fluxes and photolysis rate coefficients (J-values) for atmospheric chemistry models. It implements the Delta-Eddington two-stream approximation for radiative transfer through a plane-parallel atmosphere with spherical geometry corrections.

### What It Does

1. **Radiative Transfer**: Solves for actinic flux at each altitude and wavelength
2. **Photolysis Rates**: Calculates J-values from:
   ```
   J = integral[ sigma(lambda,T) * phi(lambda,T) * F(lambda,z) ] dlambda
   ```
   where sigma = absorption cross-section, phi = quantum yield, F = actinic flux

3. **Atmospheric Radiators**:
   - O3: Temperature-dependent absorption (Hartley/Huggins bands)
   - O2: Schumann-Runge bands (175-245 nm)
   - Rayleigh: Molecular scattering (lambda^-4 dependence)
   - Aerosol: Angstrom parameterization (configurable tau, omega, g)

4. **Supporting Infrastructure**:
   - US Standard Atmosphere 1976 profiles (T, P, rho, O3, O2)
   - Solar flux (ASTM E-490 reference spectrum)
   - Solar position calculation (SZA from date/time/location)
   - Spherical geometry (Chapman function for slant paths)
   - Surface albedo (uniform or wavelength-dependent)

## Quick Start

```cpp
#include <tuvx/model/tuv_model.hpp>

tuvx::ModelConfig config;
config.solar_zenith_angle = 30.0;
config.ozone_column_DU = 300.0;

tuvx::TuvModel model(config);
model.UseStandardAtmosphere();
model.AddStandardRadiators();  // O3 + O2 + Rayleigh

// Add photolysis reaction
tuvx::O3CrossSection o3_xs;
tuvx::O3O1DQuantumYield o3_qy;
model.AddPhotolysisReaction("O3 -> O2 + O(1D)", &o3_xs, &o3_qy);

auto output = model.Calculate();
double j_o3 = output.GetSurfacePhotolysisRate("O3 -> O2 + O(1D)");
```

## Building

```bash
mkdir build && cd build
cmake .. -DTUVX_ENABLE_TESTS=ON
make -j4
ctest --output-on-failure
```

### Requirements

- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.16+
- Google Test (fetched automatically)

## Design Philosophy

**Header-only library with embedded data** - no external file dependencies.

All spectral data (cross-sections, solar flux, quantum yields) and atmospheric profiles are embedded directly in the source code. This approach:

- Enables single-header inclusion with no runtime file I/O
- Eliminates data file path configuration issues
- Keeps the library self-contained and portable
- Simplifies deployment and testing

The embedded data uses representative values suitable for demonstrating correct radiative transfer behavior. For research applications requiring precise JPL/IUPAC recommended values, users can construct cross-sections with custom data arrays.

## Current Limitations

| Component | Current State | Full Fidelity Would Need |
|-----------|---------------|--------------------------|
| O3 cross-section | ~20 points, 200-800 nm | 63,501 points at 0.01 nm resolution |
| O2 cross-section | ~20 points, simplified S-R | High-resolution S-R band structure |
| Solar flux | ~40 points | 2000+ points |
| Photolysis reactions | O3->O(1D) implemented | 69 reactions |

The physics is correct; the spectral resolution is coarse. Good for:
- Demonstrating radiative transfer behavior
- Testing and validation
- Educational use
- Prototyping

## Test Coverage

**539 tests** covering:
- Grid/profile/interpolation infrastructure
- Cross-sections and quantum yields
- Radiator optical properties
- Delta-Eddington solver (analytical benchmarks)
- Full model scenarios (SZA dependence, altitude profiles, UV attenuation)
- Spectral analysis (cross-sections, optical depths, transmittance)

## Project Structure

```
include/tuvx/
  model/          - TuvModel, ModelConfig, ModelOutput
  solver/         - Delta-Eddington two-stream solver
  radiator/       - O3, O2, Rayleigh, aerosol radiators
  cross_section/  - Absorption cross-sections
  quantum_yield/  - Photolysis quantum yields
  grid/           - Wavelength and altitude grids
  profile/        - Atmospheric profiles
  solar/          - Solar position and ET flux
  photolysis/     - J-value calculations
  util/           - Constants, arrays, error handling

test/unit/        - Google Test unit tests
scripts/          - Python plotting utilities
```

## Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) - Component diagram and design
- [TODO.md](TODO.md) - Development roadmap and status
- [NUMERICAL-TESTS.md](NUMERICAL-TESTS.md) - Validation test specifications
- [CLAUDE.md](CLAUDE.md) - Coding standards

## Related Projects

- [TUV-x](https://github.com/NCAR/tuv-x) - Fortran implementation (reference)
- [MICM](https://github.com/NCAR/micm) - Model Independent Chemistry Module

## License

Apache 2.0 - See [LICENSE](LICENSE) file.
