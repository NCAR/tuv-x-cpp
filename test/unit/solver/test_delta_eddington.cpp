#include <tuvx/solver/delta_eddington.hpp>

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// Helper to create a simple radiator state
RadiatorState CreateSimpleState(std::size_t n_layers, std::size_t n_wavelengths,
                                 double tau, double omega, double g)
{
  RadiatorState state;
  state.Initialize(n_layers, n_wavelengths);

  for (std::size_t i = 0; i < n_layers; ++i)
  {
    for (std::size_t j = 0; j < n_wavelengths; ++j)
    {
      state.optical_depth[i][j] = tau;
      state.single_scattering_albedo[i][j] = omega;
      state.asymmetry_factor[i][j] = g;
    }
  }

  return state;
}

// ============================================================================
// Solver Construction Tests
// ============================================================================

TEST(DeltaEddingtonTest, Construction)
{
  DeltaEddingtonSolver solver;
  EXPECT_EQ(solver.Name(), "delta_eddington");
}

TEST(DeltaEddingtonTest, Clone)
{
  DeltaEddingtonSolver solver;
  auto clone = solver.Clone();

  EXPECT_NE(clone, nullptr);
  EXPECT_EQ(clone->Name(), "delta_eddington");
}

TEST(DeltaEddingtonTest, CanHandle)
{
  DeltaEddingtonSolver solver;

  EXPECT_TRUE(solver.CanHandle(0.0));    // Zenith
  EXPECT_TRUE(solver.CanHandle(60.0));   // Moderate SZA
  EXPECT_TRUE(solver.CanHandle(89.0));   // Near horizon
  EXPECT_FALSE(solver.CanHandle(90.0));  // Horizon
  EXPECT_FALSE(solver.CanHandle(95.0));  // Below horizon
}

// ============================================================================
// Empty Input Tests
// ============================================================================

TEST(DeltaEddingtonTest, EmptyInput)
{
  DeltaEddingtonSolver solver;
  SolverInput input;

  auto result = solver.Solve(input);

  EXPECT_TRUE(result.Empty());
}

TEST(DeltaEddingtonTest, EmptyRadiatorState)
{
  DeltaEddingtonSolver solver;
  RadiatorState empty_state;

  SolverInput input;
  input.radiator_state = &empty_state;
  input.solar_zenith_angle = 30.0;

  auto result = solver.Solve(input);

  EXPECT_TRUE(result.Empty());
}

// ============================================================================
// Night Time Tests
// ============================================================================

TEST(DeltaEddingtonTest, NightTime)
{
  DeltaEddingtonSolver solver;
  auto state = CreateSimpleState(3, 2, 0.5, 0.8, 0.7);

  SolverInput input;
  input.radiator_state = &state;
  input.solar_zenith_angle = 100.0;  // Night

  auto result = solver.Solve(input);

  // Should have correct dimensions but zero values
  EXPECT_FALSE(result.Empty());
  EXPECT_EQ(result.NumberOfLevels(), 4u);
  EXPECT_EQ(result.NumberOfWavelengths(), 2u);

  // All fluxes should be zero at night
  for (std::size_t i = 0; i < result.NumberOfLevels(); ++i)
  {
    for (std::size_t j = 0; j < result.NumberOfWavelengths(); ++j)
    {
      EXPECT_DOUBLE_EQ(result.direct_irradiance[i][j], 0.0);
      EXPECT_DOUBLE_EQ(result.diffuse_down[i][j], 0.0);
    }
  }
}

// ============================================================================
// Pure Absorption Tests
// ============================================================================

TEST(DeltaEddingtonTest, PureAbsorption)
{
  DeltaEddingtonSolver solver;

  // Pure absorber: omega = 0
  auto state = CreateSimpleState(3, 1, 1.0, 0.0, 0.0);

  std::vector<double> etr = { 1e15 };  // 1e15 photons/cm^2/s
  std::vector<double> albedo = { 0.0 };

  SolverInput input;
  input.radiator_state = &state;
  input.solar_zenith_angle = 0.0;  // Zenith
  input.extraterrestrial_flux = &etr;
  input.surface_albedo = &albedo;

  auto result = solver.Solve(input);

  // At zenith, mu0 = 1, so slant path = vertical path
  // Total optical depth = 3 layers * 1.0 = 3.0
  // Transmission = exp(-3.0) ≈ 0.05

  // TOA should have full direct flux
  EXPECT_NEAR(result.direct_irradiance[3][0], 1e15, 1e13);

  // Surface should have attenuated direct flux
  double expected_surface = 1e15 * std::exp(-3.0);
  EXPECT_NEAR(result.direct_irradiance[0][0], expected_surface, expected_surface * 0.1);

  // No scattering means no diffuse
  EXPECT_NEAR(result.diffuse_down[0][0], 0.0, 1e10);
}

TEST(DeltaEddingtonTest, PureAbsorptionSlantPath)
{
  DeltaEddingtonSolver solver;

  auto state = CreateSimpleState(1, 1, 1.0, 0.0, 0.0);
  std::vector<double> etr = { 1e15 };

  SolverInput input;
  input.radiator_state = &state;
  input.solar_zenith_angle = 60.0;  // SZA = 60°, mu0 = 0.5
  input.extraterrestrial_flux = &etr;

  auto result = solver.Solve(input);

  // Slant path factor = 1/cos(60°) = 2
  // Transmission = exp(-1.0 * 2) = exp(-2) ≈ 0.135
  double mu0 = 0.5;
  double expected_toa = 1e15 * mu0;  // Direct irradiance = flux * mu0
  double expected_surface = expected_toa * std::exp(-2.0);

  EXPECT_NEAR(result.direct_irradiance[1][0], expected_toa, expected_toa * 0.01);
  EXPECT_NEAR(result.direct_irradiance[0][0], expected_surface, expected_surface * 0.1);
}

// ============================================================================
// Conservative Scattering Tests
// ============================================================================

TEST(DeltaEddingtonTest, ConservativeScattering)
{
  DeltaEddingtonSolver solver;

  // Conservative scattering: omega = 1 (no absorption)
  auto state = CreateSimpleState(1, 1, 1.0, 1.0, 0.0);

  std::vector<double> etr = { 1e15 };
  std::vector<double> albedo = { 0.0 };

  SolverInput input;
  input.radiator_state = &state;
  input.solar_zenith_angle = 0.0;
  input.extraterrestrial_flux = &etr;
  input.surface_albedo = &albedo;

  auto result = solver.Solve(input);

  // With conservative scattering, energy is redistributed but not lost
  // Total should be conserved (approximately)
  double total_toa = result.direct_irradiance[1][0] + result.diffuse_down[1][0] + result.diffuse_up[1][0];
  double total_surface = result.direct_irradiance[0][0] + result.diffuse_down[0][0] + result.diffuse_up[0][0];

  // With omega=1, there should be significant diffuse component
  EXPECT_GT(result.diffuse_down[0][0], 0.0);
}

// ============================================================================
// Surface Reflection Tests
// ============================================================================

TEST(DeltaEddingtonTest, SurfaceReflection)
{
  DeltaEddingtonSolver solver;

  // Thin atmosphere so most radiation reaches surface
  auto state = CreateSimpleState(1, 1, 0.1, 0.0, 0.0);

  std::vector<double> etr = { 1e15 };
  std::vector<double> albedo_low = { 0.0 };
  std::vector<double> albedo_high = { 0.9 };

  SolverInput input;
  input.radiator_state = &state;
  input.solar_zenith_angle = 0.0;
  input.extraterrestrial_flux = &etr;

  // Low albedo
  input.surface_albedo = &albedo_low;
  auto result_low = solver.Solve(input);

  // High albedo
  input.surface_albedo = &albedo_high;
  auto result_high = solver.Solve(input);

  // Higher albedo should produce more upwelling diffuse
  EXPECT_GT(result_high.diffuse_up[0][0], result_low.diffuse_up[0][0]);
}

// ============================================================================
// Actinic Flux Tests
// ============================================================================

TEST(DeltaEddingtonTest, ActinicFluxRelationship)
{
  DeltaEddingtonSolver solver;

  auto state = CreateSimpleState(2, 1, 0.5, 0.5, 0.5);
  std::vector<double> etr = { 1e15 };

  SolverInput input;
  input.radiator_state = &state;
  input.solar_zenith_angle = 30.0;
  input.extraterrestrial_flux = &etr;

  auto result = solver.Solve(input);

  // Direct actinic flux = direct irradiance / mu0
  double mu0 = std::cos(30.0 * constants::kDegreesToRadians);

  for (std::size_t i = 0; i < result.NumberOfLevels(); ++i)
  {
    if (result.direct_irradiance[i][0] > 0)
    {
      double expected_actinic = result.direct_irradiance[i][0] / mu0;
      EXPECT_NEAR(result.actinic_flux_direct[i][0], expected_actinic, expected_actinic * 0.01);
    }
  }
}

TEST(DeltaEddingtonTest, TotalActinicFlux)
{
  DeltaEddingtonSolver solver;

  auto state = CreateSimpleState(3, 1, 0.3, 0.8, 0.6);
  std::vector<double> etr = { 1e15 };
  std::vector<double> albedo = { 0.3 };

  SolverInput input;
  input.radiator_state = &state;
  input.solar_zenith_angle = 45.0;
  input.extraterrestrial_flux = &etr;
  input.surface_albedo = &albedo;

  auto result = solver.Solve(input);

  // Total actinic flux should be sum of direct and diffuse
  auto total = result.TotalActinicFlux(0);

  EXPECT_NEAR(total[0],
              result.actinic_flux_direct[0][0] + result.actinic_flux_diffuse[0][0],
              1e10);
}

// ============================================================================
// Physical Consistency Tests
// ============================================================================

TEST(DeltaEddingtonTest, DirectFluxDecreases)
{
  DeltaEddingtonSolver solver;

  auto state = CreateSimpleState(5, 1, 0.5, 0.3, 0.5);
  std::vector<double> etr = { 1e15 };

  SolverInput input;
  input.radiator_state = &state;
  input.solar_zenith_angle = 30.0;
  input.extraterrestrial_flux = &etr;

  auto result = solver.Solve(input);

  // Direct flux should monotonically decrease from TOA to surface
  for (std::size_t i = result.NumberOfLevels() - 1; i > 0; --i)
  {
    EXPECT_GE(result.direct_irradiance[i][0], result.direct_irradiance[i - 1][0]);
  }
}

TEST(DeltaEddingtonTest, FluxesNonNegative)
{
  DeltaEddingtonSolver solver;

  auto state = CreateSimpleState(4, 3, 0.8, 0.7, 0.6);
  std::vector<double> etr = { 1e15, 8e14, 5e14 };
  std::vector<double> albedo = { 0.2, 0.3, 0.1 };

  SolverInput input;
  input.radiator_state = &state;
  input.solar_zenith_angle = 60.0;
  input.extraterrestrial_flux = &etr;
  input.surface_albedo = &albedo;

  auto result = solver.Solve(input);

  // All fluxes should be non-negative
  for (std::size_t i = 0; i < result.NumberOfLevels(); ++i)
  {
    for (std::size_t j = 0; j < result.NumberOfWavelengths(); ++j)
    {
      EXPECT_GE(result.direct_irradiance[i][j], 0.0);
      EXPECT_GE(result.diffuse_down[i][j], 0.0);
      EXPECT_GE(result.diffuse_up[i][j], 0.0);
      EXPECT_GE(result.actinic_flux_direct[i][j], 0.0);
      EXPECT_GE(result.actinic_flux_diffuse[i][j], 0.0);
    }
  }
}

TEST(DeltaEddingtonTest, MultipleWavelengths)
{
  DeltaEddingtonSolver solver;

  // Different optical depths for different wavelengths
  RadiatorState state;
  state.Initialize(2, 3);

  // UV: high absorption
  state.optical_depth[0][0] = 2.0;
  state.optical_depth[1][0] = 2.0;
  state.single_scattering_albedo[0][0] = 0.1;
  state.single_scattering_albedo[1][0] = 0.1;

  // Visible: moderate absorption
  state.optical_depth[0][1] = 0.5;
  state.optical_depth[1][1] = 0.5;
  state.single_scattering_albedo[0][1] = 0.5;
  state.single_scattering_albedo[1][1] = 0.5;

  // Near-IR: low absorption
  state.optical_depth[0][2] = 0.1;
  state.optical_depth[1][2] = 0.1;
  state.single_scattering_albedo[0][2] = 0.9;
  state.single_scattering_albedo[1][2] = 0.9;

  std::vector<double> etr = { 1e15, 1e15, 1e15 };

  SolverInput input;
  input.radiator_state = &state;
  input.solar_zenith_angle = 30.0;
  input.extraterrestrial_flux = &etr;

  auto result = solver.Solve(input);

  // UV should have lowest surface flux (most absorption)
  // Near-IR should have highest surface flux (least absorption)
  EXPECT_LT(result.direct_irradiance[0][0], result.direct_irradiance[0][1]);
  EXPECT_LT(result.direct_irradiance[0][1], result.direct_irradiance[0][2]);
}

// ============================================================================
// Integration with SphericalGeometry Tests
// ============================================================================

TEST(DeltaEddingtonTest, WithSphericalGeometry)
{
  DeltaEddingtonSolver solver;

  auto state = CreateSimpleState(3, 1, 0.5, 0.3, 0.5);
  std::vector<double> etr = { 1e15 };

  // Create spherical geometry result
  SphericalGeometry::SlantPathResult geom;
  geom.enhancement_factor = { 2.0, 1.8, 1.5 };  // Higher at surface
  geom.sunlit = { true, true, true };
  geom.zenith_angle = 60.0;

  SolverInput input;
  input.radiator_state = &state;
  input.solar_zenith_angle = 60.0;
  input.extraterrestrial_flux = &etr;
  input.geometry = &geom;

  auto result = solver.Solve(input);

  // With spherical geometry corrections, attenuation should be different
  // from simple 1/mu0
  EXPECT_FALSE(result.Empty());
  EXPECT_GT(result.direct_irradiance[0][0], 0.0);
}
