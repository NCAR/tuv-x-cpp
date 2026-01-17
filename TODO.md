# TUV-x C++ TODO

## Project Status

**Current State**: Working UV-Vis Radiative Transfer Library
**Tests**: 539 passing
**Branch**: develop

---

## Project Objectives

### Primary Goals

1. **Feature parity with TUV-x Fortran** - Implement all photolysis reactions and radiative
   transfer capabilities from the reference Fortran implementation (`/Users/fillmore/EarthSystem/TUV-x`).
   This includes:
   - 69 photolysis reactions with validated cross-sections and quantum yields
   - 28 dose rate calculations
   - High-resolution spectral data (0.01 nm resolution for O3, Schumann-Runge bands for O2)
   - Multiple solver options beyond Delta-Eddington

2. **MUSICA API integration** - Provide a C++ photolysis module compatible with the
   [MUSICA](https://github.com/NCAR/musica) (Multi-Scale Infrastructure for Chemistry and Aerosols)
   ecosystem. The library should:
   - Follow MICM coding patterns and namespace conventions (`musica::tuvx`)
   - Provide C and Fortran bindings via ISO_C_BINDING
   - Support runtime configuration for atmospheric chemistry models
   - Enable coupling with MICM for chemistry-photolysis feedback

3. **Header-only, dependency-free design** - Maintain portability and ease of integration
   by embedding all spectral data and requiring no external files at runtime.

### Success Criteria

- Photolysis rates (J-values) match TUV-x Fortran within RMS < 1e-6, max < 1e-5
- Full test coverage for all implemented reactions
- Documentation and examples for MUSICA integration
- Performance competitive with Fortran implementation

---

## Design Philosophy

**Header-only library with embedded data** - no external file dependencies.

All spectral data (cross-sections, solar flux, quantum yields) and atmospheric profiles
(US Standard Atmosphere) are embedded directly in the source code. This approach:

- Enables single-header inclusion with no runtime file I/O
- Eliminates data file path configuration issues
- Keeps the library self-contained and portable
- Simplifies deployment and testing

The embedded data uses representative values suitable for demonstrating correct
radiative transfer behavior. For research applications requiring precise JPL/IUPAC
recommended values, users can construct cross-sections with custom data arrays.

The TUV-x Fortran repository contains high-resolution NetCDF data files that could
be used to generate more accurate embedded constants if needed.

---

## Completed

### Foundation
- [x] Project structure and CMake build system
- [x] Physical constants (`tuvx::constants`)
- [x] Error handling (`error.hpp`, `internal_error.hpp`)
- [x] Array utilities (`array.hpp`)

### Data Structures
- [x] Grid system (`Grid`, `MutableGrid`, `GridWarehouse`)
- [x] Profile containers (`Profile`, `MutableProfile`, `ProfileWarehouse`)
- [x] Interpolation (`LinearInterpolator`, `ConservingInterpolator`)

### Core Components
**Cross-Sections & Quantum Yields:**
- [x] Cross-section interface and warehouse
- [x] `BaseCrossSection` - temperature-independent
- [x] `O3CrossSection` - temperature-dependent (Bass-Paur)
- [x] `O2CrossSection` - Schumann-Runge bands (simplified)
- [x] Quantum yield interface and warehouse
- [x] `ConstantQuantumYield`
- [x] `O3O1DQuantumYield` - temperature-dependent

**Radiators & Radiation Field:**
- [x] `RadiatorState` - optical properties (τ, ω, g)
- [x] `Radiator` interface and warehouse
- [x] `FromCrossSectionRadiator`
- [x] `RayleighRadiator` - molecular scattering (lambda^-4)
- [x] `AerosolRadiator` - Angstrom parameterization
- [x] `RadiationField` - actinic flux storage

**Solar & Surface:**
- [x] Solar position calculation (SZA from date/time/location)
- [x] Extraterrestrial flux (ASTM E-490 reference spectrum)
- [x] Surface albedo (uniform and wavelength-dependent)
- [x] Spherical geometry (slant paths, Chapman function)

**Solver & Photolysis:**
- [x] `Solver` interface
- [x] `DeltaEddingtonSolver` - two-stream radiative transfer
- [x] `PhotolysisRateSet` - J-value calculations

### Model Orchestration
- [x] `ModelConfig` - comprehensive configuration
- [x] `ModelOutput` - results with accessor methods
- [x] `TuvModel` - main orchestration class
- [x] `StandardAtmosphere` - US Standard Atmosphere 1976 profiles (T, P, ρ, O3, O2)
- [x] Scenario tests (clear sky, SZA dependence, polar, etc.)
- [x] `AddStandardRadiators()` convenience method (O3 + O2 + Rayleigh)

---

## Next Steps (Prioritized)

### Numerical Validation (In Progress)
See `NUMERICAL-TESTS.md` for detailed test specifications.

**Completed:**
- [x] Delta-Eddington analytical benchmarks (26 tests)
  - [x] Pure absorption (Beer-Lambert) - 0.1% tolerance
  - [x] Energy conservation tests - 10-15% tolerance (simplified solver)
  - [x] Toon-inspired qualitative tests
  - [x] Optically thin/thick limits
  - [x] Multi-layer integration
  - [x] Physical consistency
- [x] Spectral analysis tests (4 tests)
  - [x] Cross-section spectra (Rayleigh, O3, O2)
  - [x] Optical depth by radiator type
  - [x] Altitude-resolved actinic flux
  - [x] Atmospheric transmittance
- [x] Plotting infrastructure (`scripts/plot_spectral_analysis.py`)

**Fortran Parity Testing** (next priority):

Goal: Replicate TUV-x Fortran validation tests in C++

Reference Data (from TUV-x Fortran):
- `test/regression/photolysis_rates/photo_rates.nc` - 69 reactions, 125 levels × 5 SZAs
- `test/regression/dose_rates/dose_rates.nc` - 28 types, 125 levels × 5 SZAs
- Tolerances: RMS < 1e-6, max < 1e-5

Standard Test Scenario (TUV 5.4):
- Height grid: 0-120 km, 1 km resolution (121 levels)
- Wavelength grid: combined.grid
- Date: March 21, 2002 (equinox)
- Location: 0°N, 0°E
- Surface albedo: 0.1
- Atmosphere: US Standard Atmosphere 1976

Remaining work:
- [ ] Set up TUV 5.4 reference scenario
- [ ] Compare radiation field outputs
- [ ] Compare photolysis rates (69 reactions)
- [ ] Compare dose rates (28 types)
- [ ] High-resolution cross-section data from NetCDF
- [ ] High-resolution solar spectra (SUSIM, ATLAS3, SAO2010)
- [ ] NetCDF output capability
- [ ] Comparison utilities (Python or C++)

Priority Photolysis Reactions (subset for initial validation):
- O2 + hν → O + O
- O3 + hν → O2 + O(1D)
- O3 + hν → O2 + O(3P)
- NO2 + hν → NO + O(3P)
- H2O2 + hν → OH + OH
- HNO3 + hν → OH + NO2
- CH2O + hν → H + HCO
- CH2O + hν → H2 + CO

### Additional Cross-Section Types (Medium Priority)
- [ ] NO2 cross-section (temperature-dependent)
- [ ] HCHO cross-section
- [ ] H2O2 cross-section
- [ ] ClONO2 cross-section
- [ ] HNO3 cross-section
- [ ] High-resolution data loader for JPL/IUPAC recommendations

### C/Fortran Interfaces (Medium Priority)
- [ ] C API wrapper (`tuvx_c.h`)
- [ ] Fortran bindings via ISO_C_BINDING
- [ ] Example integration code

### Performance Optimization (Lower Priority)
- [ ] Profile hot paths
- [ ] SIMD vectorization for wavelength loops
- [ ] OpenMP parallelization
- [ ] Memory alignment optimization

### Configuration & I/O (Lower Priority)
- [ ] JSON configuration file support
- [ ] YAML configuration file support
- [ ] NetCDF output support (optional, for validation comparisons)

---

## Known Issues & Technical Debt

### O3 Radiator Integration (Completed)
O3 radiator is now integrated and working. The following physical behaviors are validated:
- UV-B more attenuated than UV-A
- Flux decreases from TOA to surface
- J-values increase with altitude
- Attenuation increases with SZA
- Ozone column affects UV-B attenuation

### Default vs O3-Enabled Behavior
By default, TuvModel calculates through an empty atmosphere (for backward compatibility).
To enable realistic O3 absorption, call `model.AddO3Radiator()` after construction.
The original ClearSkyScenario tests remain relaxed (empty atmosphere), while new
O3RadiatorScenario tests verify the full physical behavior.

### API Improvements
- [ ] Consider builder pattern for ModelConfig
- [ ] Add validation for cross-section/quantum yield wavelength coverage
- [ ] Add warnings for extrapolation

---

## Quick Reference

### Build Commands
```bash
cd /Users/fillmore/EarthSystem/TUV-x-cpp/build
cmake .. -DTUVX_ENABLE_TESTS=ON
make -j4
ctest --output-on-failure
```

### Key Files
- Main header: `include/tuvx/tuvx.hpp`
- Model class: `include/tuvx/model/tuv_model.hpp`
- Configuration: `include/tuvx/model/model_config.hpp`
- Solver: `include/tuvx/solver/delta_eddington.hpp`
- Tests: `test/unit/`

### Example Usage
```cpp
#include <tuvx/model/tuv_model.hpp>
#include <tuvx/cross_section/types/o3.hpp>
#include <tuvx/quantum_yield/types/o3_o1d.hpp>

tuvx::ModelConfig config;
config.solar_zenith_angle = 30.0;
config.n_wavelength_bins = 100;
config.n_altitude_layers = 80;
config.ozone_column_DU = 300.0;  // Total ozone column

tuvx::TuvModel model(config);
model.UseStandardAtmosphere();
model.AddO3Radiator();  // Enable O3 absorption

// Add photolysis reaction
tuvx::O3CrossSection o3_xs;
tuvx::O3O1DQuantumYield o3_qy;
model.AddPhotolysisReaction("O3 -> O2 + O(1D)", &o3_xs, &o3_qy);

// Calculate
auto output = model.Calculate();
double j_surface = output.GetSurfacePhotolysisRate("O3 -> O2 + O(1D)");
```

---

## References

- MICM (coding patterns): `/Users/fillmore/EarthSystem/MICM`
- TUV-x Fortran (reference): `/Users/fillmore/EarthSystem/TUV-x`
- Architecture: `ARCHITECTURE.md`
- Numerical tests: `NUMERICAL-TESTS.md`
- Coding standards: `CLAUDE.md`
