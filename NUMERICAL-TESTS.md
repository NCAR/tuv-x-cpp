# Numerical Validation Tests

This document tracks ideas and progress for numerical validation of TUV-x C++ against analytical solutions, published benchmarks, and the TUV-x Fortran reference implementation.

## Status

| Test Category | Status | Notes |
|---------------|--------|-------|
| Delta-Eddington analytical | Planned | |
| Energy conservation | Planned | |
| Photolysis J-values | Planned | |
| TUV-x Fortran parity | Planned | |

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

### 3.1 Identical Input Comparison

Create test cases with identical inputs to TUV-x Fortran:
- Same wavelength grid
- Same altitude grid
- Same atmospheric profile (T, p, O3, etc.)
- Same cross-sections and quantum yields
- Same solar flux

Compare outputs:
- Actinic flux at each level
- J-values for key reactions

### 3.2 Standard Atmosphere Cases

**US Standard Atmosphere 1976**:
- Surface to 80 km
- Standard T, p profiles
- Standard O3 column

**Tropical atmosphere**:
- Higher tropopause
- Different O3 profile

### 3.3 Tolerance Criteria

Expected agreement levels:
- Direct beam: < 0.1% difference
- Diffuse radiation: < 1% difference (method-dependent)
- J-values: < 5% difference (cumulative errors)

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

## 6. Progress Log

| Date | Test | Result | Notes |
|------|------|--------|-------|
| | | | |

