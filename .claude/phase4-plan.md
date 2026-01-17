# Phase 4: TUV Model Orchestration

## Overview

Create the main TUV model class that orchestrates all components into a complete radiative transfer and photolysis calculation.

## Architecture

```
TuvModel
  Configuration: grids, options
  Warehouses:
    - GridWarehouse (wavelength, altitude)
    - ProfileWarehouse (T, p, O3, air density, etc.)
    - CrossSectionWarehouse
    - QuantumYieldWarehouse
    - RadiatorWarehouse
  Components:
    - SolarPosition
    - ExtraterrestrialFlux
    - SurfaceAlbedo
    - SphericalGeometry
    - Solver (DeltaEddington)
  Output:
    - RadiationField
    - PhotolysisRates
```

## New Files

```
include/tuvx/
  model/
    tuv_model.hpp               - Main TUV model class
    model_config.hpp            - Configuration structures
    model_output.hpp            - Output/results structures

test/unit/
  model/
    test_tuv_model.cpp          - Model integration tests
    test_model_scenarios.cpp    - Scenario-based tests
```

## Key Classes

### ModelConfig

```cpp
struct ModelConfig {
  // Grid specifications
  GridSpec wavelength_grid;
  GridSpec altitude_grid;

  // Solar parameters
  double solar_zenith_angle{ 0.0 };  // degrees
  int day_of_year{ 1 };
  double earth_sun_distance{ 1.0 };  // AU

  // Surface
  double surface_albedo{ 0.1 };
  double surface_altitude{ 0.0 };  // km

  // Solver options
  std::string solver_type{ "delta_eddington" };
  bool spherical_geometry{ true };

  // Earth parameters
  double earth_radius{ 6371.0 };  // km
};
```

### ModelOutput

```cpp
struct ModelOutput {
  // Radiation field at all levels
  RadiationField radiation_field;

  // Photolysis rates for all reactions
  std::vector<PhotolysisRateCalculator::Result> photolysis_rates;

  // Metadata
  double solar_zenith_angle;
  bool is_daytime;

  // Accessors
  double GetPhotolysisRate(const std::string& reaction, std::size_t level) const;
  std::vector<double> GetActinicFlux(std::size_t level) const;
};
```

### TuvModel

```cpp
class TuvModel {
 public:
  // Construction
  explicit TuvModel(ModelConfig config);

  // Setup methods (fluent interface)
  TuvModel& SetWavelengthGrid(std::vector<double> edges);
  TuvModel& SetAltitudeGrid(std::vector<double> edges);
  TuvModel& SetTemperatureProfile(std::vector<double> values);
  TuvModel& SetPressureProfile(std::vector<double> values);
  TuvModel& SetOzoneProfile(std::vector<double> values);
  TuvModel& SetAirDensityProfile(std::vector<double> values);

  // Add photolysis reactions
  TuvModel& AddPhotolysisReaction(
      const std::string& name,
      const CrossSection* xs,
      const QuantumYield* qy);

  // Calculation
  ModelOutput Calculate(double solar_zenith_angle);
  ModelOutput Calculate(double latitude, double longitude,
                        int year, int month, int day,
                        double hour);

  // Access to internal components
  const GridWarehouse& Grids() const;
  const ProfileWarehouse& Profiles() const;

 private:
  ModelConfig config_;
  GridWarehouse grids_;
  ProfileWarehouse profiles_;
  CrossSectionWarehouse cross_sections_;
  QuantumYieldWarehouse quantum_yields_;
  RadiatorWarehouse radiators_;
  PhotolysisRateSet photolysis_reactions_;
  std::unique_ptr<Solver> solver_;
};
```

## Implementation Order

| Step | File | Description |
|------|------|-------------|
| 1 | `model/model_config.hpp` | Configuration structures |
| 2 | `model/model_output.hpp` | Output structures |
| 3 | `model/tuv_model.hpp` | Main TuvModel class |
| 4 | `test_tuv_model.cpp` | Basic model tests |
| 5 | `test_model_scenarios.cpp` | Scenario tests |
| 6 | Update `tuvx.hpp` | Include new headers |
| 7 | Update `CMakeLists.txt` | Add test targets |

## Test Scenarios

### Basic Calculation
- Clear sky, SZA=30°, standard atmosphere
- Verify radiation field computed
- Verify photolysis rates computed

### Night Time
- SZA=120°, verify zero radiation/J-values

### Altitude Dependence
- J-values increase with altitude (less absorption overhead)

### SZA Dependence
- J-values decrease with increasing SZA

### Multiple Reactions
- O3, NO2 photolysis simultaneously
- Verify independent calculations

## Verification

```bash
cd /Users/fillmore/EarthSystem/TUV-x-cpp/build
cmake .. -DTUVX_ENABLE_TESTS=ON
make -j4
ctest -R "TuvModel|ModelScenario" --output-on-failure
```
