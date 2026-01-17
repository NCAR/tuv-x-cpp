# TUV-x C++ TODO

## Project Status

**Current State**: Phase 7 Complete - Radiator Integration
**Tests**: 531 passing
**Branch**: develop

---

## Completed Phases

### Phase 1: Foundation
- [x] Project structure and CMake build system
- [x] Physical constants (`tuvx::constants`)
- [x] Error handling (`error.hpp`, `internal_error.hpp`)
- [x] Array utilities (`array.hpp`)

### Phase 2: Data Structures
- [x] Grid system (`Grid`, `MutableGrid`, `GridWarehouse`)
- [x] Profile containers (`Profile`, `MutableProfile`, `ProfileWarehouse`)
- [x] Interpolation (`LinearInterpolator`, `ConservingInterpolator`)

### Phase 3A: Cross-Sections & Quantum Yields
- [x] Cross-section interface and warehouse
- [x] `BaseCrossSection` - temperature-independent
- [x] `O3CrossSection` - temperature-dependent (Bass-Paur)
- [x] Quantum yield interface and warehouse
- [x] `ConstantQuantumYield`
- [x] `O3O1DQuantumYield` - temperature-dependent

### Phase 3B: Radiators & Radiation Field
- [x] `RadiatorState` - optical properties (τ, ω, g)
- [x] `Radiator` interface and warehouse
- [x] `FromCrossSectionRadiator`
- [x] `RadiationField` - actinic flux storage

### Phase 3C: Solar & Surface
- [x] Solar position calculation (SZA from date/time/location)
- [x] Extraterrestrial flux (ASTM E-490 reference spectrum)
- [x] Surface albedo (uniform and wavelength-dependent)
- [x] Spherical geometry (slant paths, Chapman function)

### Phase 3D: Solver & Photolysis
- [x] `Solver` interface
- [x] `DeltaEddingtonSolver` - two-stream radiative transfer
- [x] `PhotolysisRateSet` - J-value calculations

### Phase 4: Model Orchestration
- [x] `ModelConfig` - comprehensive configuration
- [x] `ModelOutput` - results with accessor methods
- [x] `TuvModel` - main orchestration class
- [x] `StandardAtmosphere` - US Standard Atmosphere 1976 profiles (T, P, ρ, O3)
- [x] Scenario tests (clear sky, SZA dependence, polar, etc.)
- [x] O3 radiator integration with `AddO3Radiator()` convenience method
- [x] O3 radiator scenario tests (UV attenuation, altitude/SZA dependence)

---

## Next Steps (Prioritized)

### Phase 5: Numerical Validation (In Progress)
See `NUMERICAL-TESTS.md` for detailed test specifications.

**Completed:**
- [x] Delta-Eddington analytical benchmarks (26 tests)
  - [x] Pure absorption (Beer-Lambert) - 0.1% tolerance
  - [x] Energy conservation tests - 10-15% tolerance (simplified solver)
  - [x] Toon-inspired qualitative tests
  - [x] Optically thin/thick limits
  - [x] Multi-layer integration
  - [x] Physical consistency

**Next: Fortran Parity Testing**
- [ ] Set up TUV 5.4 reference scenario
- [ ] Compare radiation field outputs
- [ ] Compare photolysis rates (69 reactions)
- [ ] Compare dose rates (28 types)

### Phase 5B: Fortran Feature Parity (High Priority)

**Goal**: Replicate TUV-x Fortran validation tests in C++

**Reference Data** (from TUV-x Fortran):
- `test/regression/photolysis_rates/photo_rates.nc` - 69 reactions, 125 levels × 5 SZAs
- `test/regression/dose_rates/dose_rates.nc` - 28 types, 125 levels × 5 SZAs
- Tolerances: RMS < 1e-6, max < 1e-5

**Standard Test Scenario** (TUV 5.4):
- Height grid: 0-120 km, 1 km resolution (121 levels)
- Wavelength grid: combined.grid
- Date: March 21, 2002 (equinox)
- Location: 0°N, 0°E
- Surface albedo: 0.1
- Atmosphere: US Standard Atmosphere 1976

**Required Components for Parity**:

1. **Cross-Sections** (Phase 6 prerequisite):
   - [ ] O2 cross-section with Schumann-Runge bands
   - [ ] O3 cross-section (temperature-dependent, 4 files)
   - [ ] Air cross-section (Rayleigh)
   - [ ] 60+ additional cross-sections for full photolysis coverage

2. **Radiators** (Phase 7 prerequisite):
   - [ ] Air/Rayleigh radiator
   - [ ] O2 radiator
   - [ ] O3 radiator
   - [ ] Aerosol radiator (with wavelength-dependent optical depth)

3. **Data Files**:
   - [ ] USSA atmosphere profiles (O3, air, O2, temperature)
   - [ ] Solar flux spectrum (SUSIM, ATLAS3, SAO2010, Neckel)
   - [ ] Cross-section NetCDF files
   - [ ] Wavelength grids

4. **Validation Infrastructure**:
   - [ ] NetCDF output capability
   - [ ] Comparison utilities (Python or C++)
   - [ ] Reference data extraction from Fortran

**Priority Photolysis Reactions** (subset for initial validation):
1. O2 + hν → O + O
2. O3 + hν → O2 + O(1D)
3. O3 + hν → O2 + O(3P)
4. NO2 + hν → NO + O(3P)
5. H2O2 + hν → OH + OH
6. HNO3 + hν → OH + NO2
7. CH2O + hν → H + HCO
8. CH2O + hν → H2 + CO

### Phase 6: Additional Cross-Section Types (Medium Priority)
- [ ] NO2 cross-section (temperature-dependent)
- [ ] O2 cross-section (Schumann-Runge bands)
- [ ] HCHO cross-section
- [ ] H2O2 cross-section
- [ ] ClONO2 cross-section
- [ ] Load from JPL/IUPAC data files

### Phase 7: Radiator Integration (Complete)
- [x] Wire radiators into TuvModel.Calculate()
- [x] Implement O3 radiator with Chapman layer O3 profile
- [x] Implement O2 radiator (Schumann-Runge bands)
- [x] Implement Rayleigh scattering radiator (λ^-4 dependence)
- [x] Implement aerosol radiator (Ångström parameterization)
- [x] Add AddStandardRadiators() convenience method (O3 + O2 + Rayleigh)

### Phase 8: C/Fortran Interfaces (Medium Priority)
- [ ] C API wrapper (`tuvx_c.h`)
- [ ] Fortran bindings via ISO_C_BINDING
- [ ] Example integration code

### Phase 9: Performance Optimization (Lower Priority)
- [ ] Profile hot paths
- [ ] SIMD vectorization for wavelength loops
- [ ] OpenMP parallelization
- [ ] Memory alignment optimization

### Phase 10: Configuration & I/O (Lower Priority)
- [ ] JSON configuration file support
- [ ] YAML configuration file support
- [ ] NetCDF output support
- [ ] Cross-section data file readers

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
