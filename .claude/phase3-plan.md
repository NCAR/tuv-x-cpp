# Phase 3: TUV-x C++ - Radiative Transfer Components

## Overview

Phase 3 implements the core radiative transfer subsystem that calculates atmospheric radiation fields and supports photolysis rate calculations. Due to complexity, this phase is divided into sub-phases.

## Sub-Phase Structure

- **Phase 3A**: Cross-sections and quantum yields framework
- **Phase 3B**: Radiators and radiation field
- **Phase 3C**: Radiative transfer solvers
- **Phase 3D**: Output calculators (photolysis, dose, heating rates)

---

# Phase 3A: Cross-Sections and Quantum Yields

## Project Structure (New Files)

```
include/tuvx/
├── cross_section/
│   ├── cross_section.hpp              # Base CrossSection class
│   ├── cross_section_warehouse.hpp    # Collection manager
│   ├── cross_section_factory.hpp      # Factory for creating from config
│   ├── temperature_based.hpp          # Temperature-dependent base
│   └── types/
│       ├── o3.hpp                     # O3 cross-section
│       ├── no2.hpp                    # NO2 cross-section
│       ├── base.hpp                   # Simple lookup table
│       └── rayleigh.hpp               # Rayleigh scattering
├── quantum_yield/
│   ├── quantum_yield.hpp              # Base QuantumYield class
│   ├── quantum_yield_warehouse.hpp    # Collection manager
│   ├── quantum_yield_factory.hpp      # Factory for creating from config
│   └── types/
│       ├── base.hpp                   # Simple constant/lookup
│       ├── o3_o1d.hpp                 # O3 -> O(1D) + O2
│       └── temperature_based.hpp      # Temperature-dependent
└── util/
    └── data_reader.hpp                # NetCDF/CSV data file reading

test/unit/
├── cross_section/
│   ├── test_cross_section.cpp
│   ├── test_cross_section_warehouse.cpp
│   └── test_cross_section_o3.cpp
└── quantum_yield/
    ├── test_quantum_yield.cpp
    └── test_quantum_yield_warehouse.cpp
```

## Key Classes

### CrossSection Base Class

```cpp
/// @brief Base class for wavelength-dependent absorption cross-sections
class CrossSection
{
 public:
  virtual ~CrossSection() = default;

  /// @brief Clone this cross-section (for polymorphic copy)
  virtual std::unique_ptr<CrossSection> Clone() const = 0;

  /// @brief Calculate cross-section values on wavelength grid
  /// @param wavelength_grid Wavelength bin midpoints [nm]
  /// @param temperature Temperature [K]
  /// @return Cross-section values [cm^2/molecule]
  virtual std::vector<double> Calculate(
      const Grid& wavelength_grid,
      double temperature) const = 0;

  /// @brief Calculate cross-section profile (altitude-dependent)
  /// @param wavelength_grid Wavelength grid
  /// @param altitude_grid Altitude grid
  /// @param temperature_profile Temperature vs altitude
  /// @return 2D array [altitude][wavelength] of cross-sections
  virtual std::vector<std::vector<double>> CalculateProfile(
      const Grid& wavelength_grid,
      const Grid& altitude_grid,
      const Profile& temperature_profile) const;

  /// @brief Get the name of this cross-section
  const std::string& Name() const { return name_; }

 protected:
  std::string name_;
  std::string units_{"cm^2"};
};
```

### QuantumYield Base Class

```cpp
/// @brief Base class for photochemical quantum yields
class QuantumYield
{
 public:
  virtual ~QuantumYield() = default;

  /// @brief Clone this quantum yield
  virtual std::unique_ptr<QuantumYield> Clone() const = 0;

  /// @brief Calculate quantum yield values
  /// @param wavelength_grid Wavelength grid
  /// @param temperature Temperature [K]
  /// @param air_density Air number density [molecules/cm^3]
  /// @return Quantum yield values [0-1]
  virtual std::vector<double> Calculate(
      const Grid& wavelength_grid,
      double temperature,
      double air_density = 0.0) const = 0;

  /// @brief Calculate quantum yield profile
  virtual std::vector<std::vector<double>> CalculateProfile(
      const Grid& wavelength_grid,
      const Grid& altitude_grid,
      const Profile& temperature_profile,
      const Profile& air_density_profile) const;

  const std::string& Name() const { return name_; }

 protected:
  std::string name_;
};
```

### Warehouse Pattern

```cpp
/// @brief Collection manager for cross-sections
class CrossSectionWarehouse
{
 public:
  using Handle = std::size_t;

  /// @brief Add a cross-section to the warehouse
  Handle Add(std::unique_ptr<CrossSection> cross_section);

  /// @brief Get cross-section by name
  const CrossSection& Get(const std::string& name) const;

  /// @brief Get cross-section by handle
  const CrossSection& Get(Handle handle) const;

  /// @brief Check if cross-section exists
  bool Exists(const std::string& name) const;

  /// @brief Get all cross-section names
  std::vector<std::string> Names() const;

 private:
  std::vector<std::unique_ptr<CrossSection>> cross_sections_;
  std::unordered_map<std::string, std::size_t> name_to_index_;
};
```

## Implementation Order for Phase 3A

| Step | File | Description |
|------|------|-------------|
| 1 | `cross_section/cross_section.hpp` | Base CrossSection class |
| 2 | `cross_section/temperature_based.hpp` | Temperature interpolation mixin |
| 3 | `cross_section/types/base.hpp` | Simple lookup table implementation |
| 4 | `test_cross_section.cpp` | Base tests |
| 5 | `cross_section/cross_section_warehouse.hpp` | Warehouse |
| 6 | `test_cross_section_warehouse.cpp` | Warehouse tests |
| 7 | `cross_section/types/o3.hpp` | O3 cross-section (important species) |
| 8 | `test_cross_section_o3.cpp` | O3 tests |
| 9 | `quantum_yield/quantum_yield.hpp` | Base QuantumYield class |
| 10 | `quantum_yield/types/base.hpp` | Simple constant/lookup |
| 11 | `test_quantum_yield.cpp` | Base tests |
| 12 | `quantum_yield/quantum_yield_warehouse.hpp` | Warehouse |
| 13 | `test_quantum_yield_warehouse.cpp` | Warehouse tests |
| 14 | `quantum_yield/types/o3_o1d.hpp` | O3 -> O(1D) quantum yield |
| 15 | Update `tuvx.hpp` | Include new headers |

## Test Strategy for Phase 3A

**Cross-Section Tests:**
- Construction and cloning
- Temperature interpolation
- Wavelength grid mapping
- Known values for O3 at standard conditions
- Profile calculation across altitude

**Quantum Yield Tests:**
- Construction and cloning
- Temperature/pressure dependence
- Air density (quenching) effects
- Profile calculation

---

# Phase 3B: Radiators and Radiation Field

## Project Structure

```
include/tuvx/
├── radiator/
│   ├── radiator.hpp                   # Base Radiator class
│   ├── radiator_state.hpp             # Optical properties state
│   ├── radiator_warehouse.hpp         # Collection manager
│   └── types/
│       ├── from_cross_section.hpp     # Molecular absorber
│       ├── aerosol.hpp                # Aerosol radiator
│       └── rayleigh.hpp               # Rayleigh scattering
└── radiation_field/
    └── radiation_field.hpp            # Computed radiation quantities

test/unit/
├── radiator/
│   ├── test_radiator.cpp
│   ├── test_radiator_state.cpp
│   └── test_radiator_warehouse.cpp
└── radiation_field/
    └── test_radiation_field.cpp
```

## Key Classes

### RadiatorState

```cpp
/// @brief Optical properties for a radiator on a grid
struct RadiatorState
{
  /// @brief Layer optical depths [n_layers][n_wavelengths]
  std::vector<std::vector<double>> optical_depth;

  /// @brief Single scattering albedo [n_layers][n_wavelengths]
  std::vector<std::vector<double>> single_scattering_albedo;

  /// @brief Asymmetry factor [n_layers][n_wavelengths]
  std::vector<std::vector<double>> asymmetry_factor;

  /// @brief Accumulate another state into this one
  void Accumulate(const RadiatorState& other);

  /// @brief Initialize with zeros for given dimensions
  void Initialize(std::size_t n_layers, std::size_t n_wavelengths);
};
```

### Radiator

```cpp
/// @brief Atmospheric constituent affecting radiative transfer
class Radiator
{
 public:
  virtual ~Radiator() = default;

  /// @brief Clone this radiator
  virtual std::unique_ptr<Radiator> Clone() const = 0;

  /// @brief Update optical state for current conditions
  /// @param grids Available grids (wavelength, altitude)
  /// @param profiles Atmospheric profiles (density, temperature)
  virtual void UpdateState(
      const GridWarehouse& grids,
      const ProfileWarehouse& profiles) = 0;

  /// @brief Get current optical state
  const RadiatorState& State() const { return state_; }

  const std::string& Name() const { return name_; }

 protected:
  std::string name_;
  RadiatorState state_;
};
```

### RadiationField

```cpp
/// @brief Computed radiation quantities at all altitudes/wavelengths
class RadiationField
{
 public:
  /// @brief Direct spectral irradiance (downwelling) [n_levels][n_wavelengths]
  std::vector<std::vector<double>> direct_irradiance;

  /// @brief Diffuse upwelling irradiance [n_levels][n_wavelengths]
  std::vector<std::vector<double>> diffuse_up;

  /// @brief Diffuse downwelling irradiance [n_levels][n_wavelengths]
  std::vector<std::vector<double>> diffuse_down;

  /// @brief Direct actinic flux [n_levels][n_wavelengths]
  std::vector<std::vector<double>> actinic_flux_direct;

  /// @brief Diffuse actinic flux (up + down) [n_levels][n_wavelengths]
  std::vector<std::vector<double>> actinic_flux_diffuse;

  /// @brief Total actinic flux (direct + diffuse)
  std::vector<double> TotalActinicFlux(std::size_t level_index) const;

  /// @brief Apply scale factor (e.g., for time averaging)
  void Scale(double factor);

  /// @brief Initialize with zeros
  void Initialize(std::size_t n_levels, std::size_t n_wavelengths);
};
```

## Implementation Order for Phase 3B

| Step | File | Description |
|------|------|-------------|
| 1 | `radiator/radiator_state.hpp` | RadiatorState struct |
| 2 | `test_radiator_state.cpp` | State accumulation tests |
| 3 | `radiator/radiator.hpp` | Base Radiator class |
| 4 | `radiator/types/from_cross_section.hpp` | Molecular absorber |
| 5 | `test_radiator.cpp` | Radiator tests |
| 6 | `radiator/radiator_warehouse.hpp` | Warehouse |
| 7 | `test_radiator_warehouse.cpp` | Warehouse tests |
| 8 | `radiation_field/radiation_field.hpp` | RadiationField class |
| 9 | `test_radiation_field.cpp` | Radiation field tests |

---

# Phase 3C: Radiative Transfer Solvers

## Project Structure

```
include/tuvx/
├── solver/
│   ├── solver.hpp                     # Abstract solver interface
│   ├── delta_eddington.hpp            # Two-stream solver
│   └── spherical_geometry.hpp         # Slant path calculations
└── util/
    └── linear_algebra.hpp             # Tridiagonal solver

test/unit/
├── solver/
│   ├── test_delta_eddington.cpp
│   └── test_spherical_geometry.cpp
└── util/
    └── test_linear_algebra.cpp
```

## Key Classes

### Solver Interface

```cpp
/// @brief Abstract interface for radiative transfer solvers
class Solver
{
 public:
  virtual ~Solver() = default;

  /// @brief Calculate radiation field for given conditions
  /// @param solar_zenith_angle Solar zenith angle [degrees]
  /// @param wavelength_grid Wavelength grid
  /// @param altitude_grid Altitude grid
  /// @param atmospheric_state Combined optical properties
  /// @param surface_albedo Surface albedo [0-1]
  /// @param extraterrestrial_flux TOA solar flux [photons/cm^2/s/nm]
  /// @return Computed radiation field
  virtual RadiationField Solve(
      double solar_zenith_angle,
      const Grid& wavelength_grid,
      const Grid& altitude_grid,
      const RadiatorState& atmospheric_state,
      const std::vector<double>& surface_albedo,
      const std::vector<double>& extraterrestrial_flux) const = 0;
};
```

### Delta-Eddington Solver

```cpp
/// @brief Two-stream Delta-Eddington radiative transfer solver
///
/// Implements the Toon et al. (1989) two-stream method with
/// delta-Eddington phase function approximation.
class DeltaEddingtonSolver : public Solver
{
 public:
  DeltaEddingtonSolver() = default;

  RadiationField Solve(
      double solar_zenith_angle,
      const Grid& wavelength_grid,
      const Grid& altitude_grid,
      const RadiatorState& atmospheric_state,
      const std::vector<double>& surface_albedo,
      const std::vector<double>& extraterrestrial_flux) const override;

 private:
  /// @brief Apply delta-Eddington scaling to optical properties
  void ApplyDeltaScaling(
      double& tau, double& omega, double& g) const;

  /// @brief Build tridiagonal system for two-stream equations
  void BuildSystem(
      std::size_t n_layers,
      const std::vector<double>& tau,
      const std::vector<double>& omega,
      const std::vector<double>& g,
      double mu0,
      double surface_albedo,
      std::vector<double>& a,
      std::vector<double>& b,
      std::vector<double>& c,
      std::vector<double>& d) const;
};
```

### SphericalGeometry

```cpp
/// @brief Spherical geometry calculations for slant paths
class SphericalGeometry
{
 public:
  /// @brief Calculate air mass factors for spherical atmosphere
  /// @param solar_zenith_angle Solar zenith angle [degrees]
  /// @param altitude_grid Altitude grid
  /// @param earth_radius Earth radius [same units as altitude]
  void Calculate(
      double solar_zenith_angle,
      const Grid& altitude_grid,
      double earth_radius = 6371.0);  // km

  /// @brief Get slant path enhancement factor for layer
  double SlantFactor(std::size_t layer_index) const;

  /// @brief Get number of layers traversed from level
  std::size_t LayersCrossed(std::size_t level_index) const;

 private:
  std::vector<double> slant_factors_;
  std::vector<std::size_t> layers_crossed_;
};
```

### Linear Algebra Utilities

```cpp
namespace linear_algebra
{
  /// @brief Solve tridiagonal system Ax = d
  /// @param a Sub-diagonal [n-1]
  /// @param b Main diagonal [n]
  /// @param c Super-diagonal [n-1]
  /// @param d Right-hand side [n]
  /// @return Solution vector x [n]
  std::vector<double> SolveTridiagonal(
      const std::vector<double>& a,
      const std::vector<double>& b,
      const std::vector<double>& c,
      const std::vector<double>& d);
}
```

## Implementation Order for Phase 3C

| Step | File | Description |
|------|------|-------------|
| 1 | `util/linear_algebra.hpp` | Tridiagonal solver |
| 2 | `test_linear_algebra.cpp` | Linear algebra tests |
| 3 | `solver/spherical_geometry.hpp` | Spherical geometry |
| 4 | `test_spherical_geometry.cpp` | Geometry tests |
| 5 | `solver/solver.hpp` | Abstract solver interface |
| 6 | `solver/delta_eddington.hpp` | Delta-Eddington implementation |
| 7 | `test_delta_eddington.cpp` | Solver tests with reference cases |

---

# Phase 3D: Output Calculators

## Project Structure

```
include/tuvx/
├── output/
│   ├── photolysis_rate.hpp            # J-value calculator
│   ├── photolysis_rate_warehouse.hpp  # Collection of reactions
│   ├── dose_rate.hpp                  # UV dose calculator
│   ├── spectral_weight.hpp            # Weighting functions
│   └── spectral_weight_warehouse.hpp  # Collection of weights
└── special/
    └── lyman_alpha_sr_bands.hpp       # O2 band corrections

test/unit/
├── output/
│   ├── test_photolysis_rate.cpp
│   ├── test_dose_rate.cpp
│   └── test_spectral_weight.cpp
└── special/
    └── test_lyman_alpha_sr_bands.cpp
```

## Key Classes

### PhotolysisRate

```cpp
/// @brief Calculator for photolysis rate constants (J-values)
class PhotolysisRate
{
 public:
  PhotolysisRate(
      std::string name,
      const CrossSection& cross_section,
      const QuantumYield& quantum_yield,
      double scaling_factor = 1.0);

  /// @brief Calculate J-value at each altitude
  /// @param wavelength_grid Wavelength grid
  /// @param altitude_grid Altitude grid
  /// @param temperature_profile Temperature profile
  /// @param radiation_field Computed radiation field
  /// @return J-values at each altitude level [1/s]
  std::vector<double> Calculate(
      const Grid& wavelength_grid,
      const Grid& altitude_grid,
      const Profile& temperature_profile,
      const RadiationField& radiation_field) const;

  const std::string& Name() const { return name_; }

 private:
  std::string name_;
  const CrossSection* cross_section_;
  const QuantumYield* quantum_yield_;
  double scaling_factor_;
};
```

### SpectralWeight

```cpp
/// @brief Spectral weighting function for dose rate calculations
class SpectralWeight
{
 public:
  virtual ~SpectralWeight() = default;

  /// @brief Clone this weight function
  virtual std::unique_ptr<SpectralWeight> Clone() const = 0;

  /// @brief Get weight values on wavelength grid
  virtual std::vector<double> Calculate(
      const Grid& wavelength_grid) const = 0;

  const std::string& Name() const { return name_; }

 protected:
  std::string name_;
};

/// @brief Standard human erythema (McKinlay-Diffey)
class ErythemalWeight : public SpectralWeight { ... };

/// @brief UV Index weight
class UVIndexWeight : public SpectralWeight { ... };

/// @brief Photosynthetically active radiation
class PARWeight : public SpectralWeight { ... };
```

### DoseRate

```cpp
/// @brief UV dose rate calculator
class DoseRate
{
 public:
  DoseRate(std::string name, const SpectralWeight& weight);

  /// @brief Calculate dose rate at each altitude
  /// @param wavelength_grid Wavelength grid
  /// @param radiation_field Computed radiation field
  /// @return Dose rate at each level [appropriate units]
  std::vector<double> Calculate(
      const Grid& wavelength_grid,
      const RadiationField& radiation_field) const;

 private:
  std::string name_;
  const SpectralWeight* weight_;
};
```

## Implementation Order for Phase 3D

| Step | File | Description |
|------|------|-------------|
| 1 | `output/spectral_weight.hpp` | Base weight class |
| 2 | `test_spectral_weight.cpp` | Weight tests |
| 3 | `output/spectral_weight_warehouse.hpp` | Warehouse |
| 4 | `output/dose_rate.hpp` | Dose rate calculator |
| 5 | `test_dose_rate.cpp` | Dose rate tests |
| 6 | `output/photolysis_rate.hpp` | J-value calculator |
| 7 | `test_photolysis_rate.cpp` | Photolysis tests |
| 8 | `output/photolysis_rate_warehouse.hpp` | Warehouse |
| 9 | `special/lyman_alpha_sr_bands.hpp` | O2 band corrections |
| 10 | `test_lyman_alpha_sr_bands.cpp` | Band correction tests |

---

## Verification

```bash
# Build
cd /Users/fillmore/EarthSystem/TUV-x-cpp/build
cmake .. -DTUVX_ENABLE_TESTS=ON
make -j4

# Run all tests
ctest --output-on-failure

# Run Phase 3 tests
ctest -R "CrossSection|QuantumYield|Radiator|Solver|Photolysis|DoseRate" --output-on-failure
```

## Dependencies

- Uses Phase 2 Grid, Profile, and Warehouse patterns
- Uses Phase 1 error handling and constants
- May need external library for NetCDF (data files) - deferred to later
- Linear algebra is self-contained (tridiagonal solver)

## Integration Points with Host Applications

- `RadiatorWarehouse` accepts radiators from external sources
- `CrossSectionWarehouse` can load from data files
- `Solver::Solve()` takes standard inputs, returns standard outputs
- `PhotolysisRateWarehouse` provides J-values for chemistry models

## Notes

- Header-only where possible, implementation files for complex algorithms
- Start with Delta-Eddington solver; discrete ordinant is optional
- Begin with ~5 cross-section types; expand as needed
- Validate against TUV-x Fortran reference outputs
