# Numerical Validation Tests

This document tracks ideas and progress for numerical validation of TUV-x C++ against analytical solutions, published benchmarks, and the TUV-x Fortran reference implementation.

## Status

| Test Category | Status | Notes |
|---------------|--------|-------|
| Delta-Eddington analytical | **Implemented** | Beer-Lambert tests with 0.1% tolerance |
| Energy conservation | **Implemented** | Conservative scattering R+T≈1 tests |
| Toon et al. (1989) benchmarks | **Implemented** | Qualitative tests for simplified solver |
| Optically thin/thick limits | **Implemented** | Asymptotic behavior verification |
| Multi-layer integration | **Implemented** | Layer discretization consistency |
| Physical consistency | **Implemented** | Asymmetry, albedo, omega effects |
| Fortran parity infrastructure | Planned | NetCDF reader, comparison utils |
| Photolysis J-values (69 reactions) | Planned | Requires cross-sections, radiators |
| Dose rates (28 types) | Planned | Requires spectral weights |
| Radiation field validation | Planned | Requires full radiator stack |

---

## 1. Delta-Eddington Solver Validation

### 1.1 Pure Absorption (Beer-Lambert Law)

**Test case**: Single layer, no scattering (ω=0), verify exponential attenuation.

```
Input:
  - τ = 1.0 (optical depth)
  - ω = 0.0 (single scattering albedo)
  - μ₀ = 1.0 (zenith sun)
  - F₀ = 1.0 (incident flux)

Expected:
  - Direct transmission: T = exp(-τ/μ₀) = exp(-1) ≈ 0.3679
  - No diffuse radiation
```

**Slant path variant**: Test with μ₀ = 0.5 (SZA = 60°), expect T = exp(-2) ≈ 0.1353

### 1.2 Conservative Scattering (Energy Conservation)

**Test case**: ω = 1.0 (no absorption), verify energy conservation.

```
Input:
  - τ = 1.0
  - ω = 1.0
  - g = 0.0 (isotropic scattering)
  - Surface albedo = 0.0

Expected:
  - Absorbed + Reflected + Transmitted = Incident
  - With ω=1: Absorbed = 0, so Reflected + Transmitted = Incident
```

### 1.3 Toon et al. (1989) Benchmark Cases

Reference: Toon, O.B., et al., 1989, J. Geophys. Res., 94, 16287-16301.

**Table 1 reproduction**: Standard test cases for two-stream methods.

| Case | τ | ω | g | μ₀ | A_s | Expected R | Expected T |
|------|---|---|---|-----|-----|------------|------------|
| 1 | 1.0 | 0.0 | 0.0 | 1.0 | 0.0 | 0.0 | 0.368 |
| 2 | 1.0 | 1.0 | 0.0 | 1.0 | 0.0 | TBD | TBD |
| 3 | 1.0 | 0.9 | 0.75 | 0.5 | 0.0 | TBD | TBD |

### 1.4 Rayleigh Scattering Limit

**Test case**: Small particles, g → 0, ω determined by molecular properties.

```
At 300 nm in pure Rayleigh atmosphere:
  - g ≈ 0 (symmetric phase function)
  - ω ≈ 1.0 (negligible absorption)
  - τ_Rayleigh ∝ λ⁻⁴
```

### 1.5 Optically Thin/Thick Limits

**Thin limit** (τ << 1):
- Direct transmission ≈ 1 - τ/μ₀
- Diffuse << Direct

**Thick limit** (τ >> 1):
- Direct transmission → 0
- Diffuse dominates
- Asymptotic diffusion regime

---

## 2. Photolysis Rate Validation

### 2.1 O3 Photolysis - J(O¹D)

**Standard atmosphere test**: Compare against published values.

Reference values (surface, SZA=30°, clear sky):
- J(O¹D) ≈ 2-4 × 10⁻⁵ s⁻¹ (troposphere)
- J(O¹D) ≈ 1-3 × 10⁻⁴ s⁻¹ (stratosphere, 30 km)

Sources:
- Madronich, S., 1987, J. Geophys. Res., 92, 9740-9752
- JPL Publication 19-5 (NASA/JPL kinetics evaluation)

### 2.2 NO2 Photolysis - J(NO2)

Reference values (surface, SZA=30°, clear sky):
- J(NO2) ≈ 8-10 × 10⁻³ s⁻¹

### 2.3 Vertical Profile Shape

**Test**: J-values should decrease from TOA to surface due to:
1. Absorption by overhead O3 (UV)
2. Rayleigh scattering
3. Aerosol extinction

Verify monotonic decrease for UV wavelengths in clear atmosphere.

### 2.4 SZA Dependence

**Test**: J-values should decrease with increasing SZA.

```
Approximate relationship for optically thin case:
  J(θ) ≈ J(0) × cos(θ)

For optically thick (strong absorption):
  J(θ) decreases faster than cos(θ)
```

---

## 3. TUV-x Fortran Parity Tests

### 3.1 Reference Data Location

TUV-x Fortran reference data at `/Users/fillmore/EarthSystem/TUV-x/`:

| File | Contents | Dimensions |
|------|----------|------------|
| `test/regression/photolysis_rates/photo_rates.nc` | 69 photolysis rates | 125 levels × 5 SZAs |
| `test/regression/dose_rates/dose_rates.nc` | 28 dose rates | 125 levels × 5 SZAs |
| `test/regression/radiators/radField.*.nc` | Radiation field components | varies |

### 3.2 Standard Test Scenario (TUV 5.4)

Configuration from `examples/tuv_5_4.json`:

```
Height grid:     0-120 km, 1 km resolution (121 levels)
Wavelength grid: data/grids/wavelength/combined.grid
Date:            March 21, 2002 (vernal equinox)
Location:        0°N, 0°E (equator)
Time points:     12:00, 14:00 local
Surface albedo:  0.1 (uniform)
Atmosphere:      US Standard Atmosphere 1976

Profiles:
- O3:          data/profiles/atmosphere/ussa.ozone
- Air density: data/profiles/atmosphere/ussa.dens
- Temperature: data/profiles/atmosphere/ussa.temp

Solar flux:
- SUSIM (hi-res)
- ATLAS3 1994
- SAO2010
- Neckel

Radiators:
- Air (Rayleigh scattering)
- O2 (with Schumann-Runge bands)
- O3 (temperature-dependent cross-section)
- Aerosols (550nm OD=0.235, SSA=0.99, g=0.61)
```

### 3.3 Photolysis Reactions (69 total)

**Priority reactions for initial validation:**

| # | Reaction | Notes |
|---|----------|-------|
| 1 | O2 + hν → O + O | Schumann-Runge bands |
| 2 | O3 + hν → O2 + O(1D) | Primary O3 photolysis |
| 3 | O3 + hν → O2 + O(3P) | Secondary O3 photolysis |
| 4 | NO2 + hν → NO + O(3P) | NOx chemistry |
| 5 | H2O2 + hν → OH + OH | HOx source |
| 6 | HNO3 + hν → OH + NO2 | NOy chemistry |
| 7 | CH2O + hν → H + HCO | Formaldehyde radical |
| 8 | CH2O + hν → H2 + CO | Formaldehyde molecular |

**Additional reactions by category:**

- **Nitrogen oxides (12):** HO2, HNO2, HNO4, N2O, N2O5, NO3
- **Organic peroxides (4):** CH3OOH, HOCH2OOH, CH3COOOH
- **Organic nitrates (10):** CH3ONO2, C2H5ONO2, nC3H7ONO2, etc.
- **PANs (4):** CH3(OONO2), CH3CO(OONO2), CH3CH2CO(OONO2)
- **Aldehydes (10):** CH3CHO, C2H5CHO, CH2=CHCHO, HOCH2CHO, etc.
- **Ketones (6):** CH3COCH3, CH3COCH2CH3, CHOCHO, etc.
- **Halogens (20):** Cl2, ClO, OClO, ClOOCl, HOCl, ClONO2, CCl4, CFCs, etc.
- **Aqueous (3):** NO3-(aq) reactions

### 3.4 Dose Rates (28 total)

| Category | Types |
|----------|-------|
| UV bands | UV-A (315-400nm), UV-B (280-315nm), UV-B* (280-320nm), visible (>400nm) |
| Gaussian | 305nm, 320nm, 340nm, 380nm (10nm FWHM) |
| Instruments | RB Meter 501, Eppley UV Photometer |
| Biological | DNA damage, erythema (human), SCUP-mice, SCUP-human, UV index |
| Environmental | Phytoplankton, PAR (400-700nm), plant damage |
| Safety | ACGIH TLV, CIE standards |
| Medical | Previtamin-D3, NMSC |

### 3.5 Tolerance Criteria

From `xsqy.compare.json` and `sw.compare.json`:

| Quantity | RMS Tolerance | Max Tolerance |
|----------|---------------|---------------|
| Photolysis rates (most) | 1.0e-6 | 1.0e-5 |
| Dose rates (most) | 1.0e-6 | 1.0e-5 |
| DNA damage, erythema | 1.0e-4 | 1.0e-4 |
| Radiation fields | 1.0e-4 to 1.0e-3 | varies |

### 3.6 Implementation Roadmap

**Phase A: Infrastructure**
- [ ] Copy reference NetCDF files to `test/data/fortran_reference/`
- [ ] Implement NetCDF reader for reference data
- [ ] Create comparison utility class
- [ ] Set up parameterized test framework

**Phase B: Radiation Field Validation**
- [ ] Import USSA atmosphere profiles
- [ ] Configure identical radiator stack (air, O2, O3, aerosols)
- [ ] Compare actinic flux profiles at 5 SZAs
- [ ] Compare direct/diffuse irradiance

**Phase C: Photolysis Rate Validation**
- [ ] Implement priority cross-sections (8 reactions)
- [ ] Compare J-values against photo_rates.nc
- [ ] Extend to full 69 reactions

**Phase D: Dose Rate Validation**
- [ ] Implement spectral weight functions
- [ ] Compare dose rates against dose_rates.nc

### 3.7 Data File Requirements

Files to copy/link from TUV-x Fortran:

```
From TUV-x/data/:
├── cross_sections/
│   ├── O2_1.nc, O2_parameters.txt
│   ├── O3_1.nc, O3_2.nc, O3_3.nc, O3_4.nc
│   └── [60+ additional cross-section files]
├── grids/wavelength/
│   └── combined.grid
├── profiles/
│   ├── atmosphere/ussa.{ozone,dens,temp}
│   └── solar/*.flx
└── quantum_yields/
    └── [quantum yield files]

From TUV-x/test/regression/:
└── photolysis_rates/photo_rates.nc
└── dose_rates/dose_rates.nc
```

---

## 4. Spherical Geometry Validation

### 4.1 Chapman Function

Compare computed air mass factors against tabulated Chapman function values.

Reference: Smith & Smith, 1972, J. Geophys. Res., 77, 3592-3597.

### 4.2 Twilight Calculations

**Test case**: SZA = 90-96°, verify:
- Upper atmosphere still illuminated
- Correct screening height calculation
- Smooth transition across terminator

### 4.3 Refraction Effects

Future: Compare against ray-tracing results that include atmospheric refraction.

---

## 5. Implementation Notes

### Test Data Files

Store reference data in `test/data/numerical/`:
```
test/data/numerical/
├── toon1989_benchmarks.csv
├── jpl_cross_sections/
├── standard_atmospheres/
│   ├── us_standard_1976.csv
│   └── tropical.csv
└── reference_jvalues/
    └── madronich_1987.csv
```

### Test Framework

Use Google Test with parameterized tests for systematic validation:

```cpp
class NumericalValidationTest : public ::testing::TestWithParam<BenchmarkCase> {};

TEST_P(NumericalValidationTest, ToonBenchmark) {
  auto params = GetParam();
  auto result = solver.Solve(params.input);
  EXPECT_NEAR(result.reflectance, params.expected_R, params.tolerance);
  EXPECT_NEAR(result.transmittance, params.expected_T, params.tolerance);
}
```

### Continuous Integration

Add numerical validation as separate test target:
```cmake
create_tuvx_test(test_numerical_validation numerical/test_validation.cpp)
```

Run with extended timeout for comprehensive benchmarks.

---

## 6. Visualization and Plotting

### 6.1 CSV Output from Tests

The benchmark tests support `--csv` command-line flag to output results for plotting:

```bash
# Generate CSV data
./build/test/unit/test_delta_eddington_benchmarks --csv \
    --gtest_filter="*BeerLambert*:*EnergyConservation*" 2>/dev/null \
    | grep -v "^\[" | grep -v "^Note:" > data/benchmarks.csv
```

**CSV Format:**
```
test_name,tau,omega,g,mu0,surface_albedo,expected_T,actual_T,expected_R,actual_R,rel_error_T,rel_error_R
```

### 6.2 Plotting Scripts

Python plotting scripts in `scripts/` directory (requires `mpas` conda environment):

```bash
conda activate mpas
python scripts/plot_solver_benchmarks.py data/benchmarks.csv -o data/benchmark
```

**Generated Plots:**
- `benchmark_beer_lambert.{png,pdf}` - Expected vs actual transmittance, relative errors
- `benchmark_energy_conservation.{png,pdf}` - R + T energy balance for ω=1 cases
- `benchmark_parameter_space.{png,pdf}` - T vs τ, T vs μ₀, R vs T summary

**Features:**
- Seaborn styling with DejaVu Sans font
- LaTeX math rendering ($\tau$, $\mu_0$, $\omega$, etc.)
- 300 DPI PNG and vector PDF output
- Automatic handling of machine-precision errors
- Summary statistics panel

---

## 7. Progress Log

| Date | Test | Result | Notes |
|------|------|--------|-------|
| 2026-01-17 | Delta-Eddington benchmarks | **Passing** | 26 tests: Beer-Lambert (6), energy conservation (5), Toon-inspired (5), thin/thick limits (4), multi-layer (3), physical consistency (3). Total test count: 519 |
| 2026-01-17 | CSV output + plotting | **Implemented** | `--csv` flag, Python plotting scripts with seaborn |

### Implemented Test Summary

**File:** `test/unit/validation/test_delta_eddington_benchmarks.cpp`

**Note on Solver Characteristics:**
The current Delta-Eddington solver uses a simplified single-scattering approximation rather than a full two-stream solution. Tests are designed to validate:
1. Exact analytical behavior for pure absorption (Beer-Lambert)
2. Qualitatively correct behavior for scattering cases
3. Proper delta-M scaling of optical properties
4. Numerical stability across parameter ranges

**Section 1: Beer-Lambert Analytical (0.1% tolerance)**
- `BeerLambert_UnitOpticalDepth`: tau=1, SZA=0, T=exp(-1) = 0.3679
- `BeerLambert_SlantPath60`: tau=1, SZA=60 (mu0=0.5), T=exp(-2) = 0.1353
- `BeerLambert_SlantPath75`: tau=1, SZA=75, T=exp(-tau/mu0)
- `BeerLambert_ThickLayer`: tau=5, SZA=0, T=exp(-5) = 0.0067
- `BeerLambert_ThinLayer`: tau=0.1, SZA=0, T=exp(-0.1) = 0.9048
- `BeerLambert_MultiLayer`: 4x tau=0.5, SZA=0, T=exp(-2) = 0.1353

**Section 2: Energy Conservation (10-15% tolerance)**
- `EnergyConservation_Isotropic`: omega=1, g=0, verify R+T ≈ 1
- `EnergyConservation_Forward`: omega=1, g=0.5, verify R+T ≈ 1
- `EnergyConservation_Backward`: omega=1, g=-0.3, verify R+T ≈ 1
- `EnergyConservation_WithSurface`: omega=1, albedo=0.5, verify R+T ≈ 1
- `EnergyConservation_SlantPath`: omega=1, SZA=60, verify R+T ≈ 1

**Section 3: Toon-Inspired Qualitative Tests**
- `Toon_PureAbsorption`: tau=1, omega=0, verify Beer-Lambert exact
- `Toon_ConservativeIsotropic`: tau=1, omega=1, g=0, verify T > exp(-tau)
- `Toon_ForwardScatterSlant`: tau=1, omega=0.9, g=0.75, verify T > R
- `Toon_WithSurfaceAlbedo`: verify surface increases TOA reflectance
- `Toon_OpticallyThick`: tau=10, verify delta-M scaled attenuation

**Section 4: Optically Thin/Thick Limits**
- `ThinLimit_DirectDominates`: tau=0.01, verify diffuse << direct
- `ThinLimit_WeakScattering`: tau=0.001, verify diffuse negligible
- `ThickLimit_DirectVanishes`: tau=50, verify T_direct < 1e-10
- `ThickLimit_DiffuseDominates`: tau=20, omega=0.99, verify diffuse > direct

**Section 5: Multi-layer Integration**
- `MultiLayer_MatchesSingleLayer`: 10x tau=0.1 matches 1x tau=1.0 for absorption
- `MultiLayer_MatchesSingleLayer_WithScattering`: verify direct matches, diffuse reasonable
- `MultiLayer_DirectAttenuation_Monotonic`: verify direct decreases from TOA to surface

**Section 6: Physical Consistency**
- `AsymmetryFactor_AffectsDiffuseDistribution`: different g produces different results
- `SurfaceAlbedo_IncreasesUpwelling`: higher albedo increases upwelling
- `HigherOmega_MoreScattering`: higher omega produces more diffuse radiation

