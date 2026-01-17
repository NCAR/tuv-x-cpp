#include <tuvx/cross_section/types/base.hpp>
#include <tuvx/grid/grid.hpp>
#include <tuvx/grid/grid_warehouse.hpp>
#include <tuvx/profile/profile.hpp>
#include <tuvx/profile/profile_warehouse.hpp>
#include <tuvx/radiator/radiator.hpp>
#include <tuvx/radiator/types/from_cross_section.hpp>

#include <cmath>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// Helper to create a simple cross-section
std::unique_ptr<CrossSection> MakeSimpleCrossSection(const std::string& name, double xs_value)
{
  std::vector<double> wavelengths = { 200.0, 300.0, 400.0 };
  std::vector<double> xs_values = { xs_value, xs_value, xs_value };
  return std::make_unique<BaseCrossSection>(name, wavelengths, xs_values);
}

// Helper to set up grids and profiles for testing
class RadiatorTestFixture : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    // Create wavelength grid
    GridSpec wl_spec{ "wavelength", "nm", 3 };
    std::vector<double> wl_edges = { 150.0, 250.0, 350.0, 450.0 };
    grids_.Add(Grid(wl_spec, wl_edges));

    // Create altitude grid (3 layers, 10 km each)
    GridSpec alt_spec{ "altitude", "km", 3 };
    std::vector<double> alt_edges = { 0.0, 10.0, 20.0, 30.0 };
    grids_.Add(Grid(alt_spec, alt_edges));

    // Create temperature profile
    ProfileSpec temp_spec{ "temperature", "K", 3 };
    std::vector<double> temp_values = { 288.0, 250.0, 220.0 };
    profiles_.Add(Profile(temp_spec, temp_values));

    // Create O3 density profile (molecules/cm^3)
    ProfileSpec o3_spec{ "O3", "molecules/cm^3", 3 };
    std::vector<double> o3_values = { 1e12, 5e12, 1e13 };  // Peak in stratosphere
    profiles_.Add(Profile(o3_spec, o3_values));
  }

  GridWarehouse grids_;
  ProfileWarehouse profiles_;
};

// ============================================================================
// FromCrossSectionRadiator Construction Tests
// ============================================================================

TEST_F(RadiatorTestFixture, Construction)
{
  auto xs = MakeSimpleCrossSection("O3", 1e-18);

  FromCrossSectionRadiator radiator("O3", std::move(xs), "O3", "temperature", "wavelength", "altitude");

  EXPECT_EQ(radiator.Name(), "O3");
  EXPECT_FALSE(radiator.HasState());
  EXPECT_EQ(radiator.DensityProfileName(), "O3");
}

TEST_F(RadiatorTestFixture, Clone)
{
  auto xs = MakeSimpleCrossSection("O3", 1e-18);
  FromCrossSectionRadiator original("O3", std::move(xs), "O3");

  auto clone = original.Clone();

  EXPECT_EQ(clone->Name(), "O3");
  EXPECT_FALSE(clone->HasState());

  // Verify it's a FromCrossSectionRadiator
  auto* from_xs = dynamic_cast<FromCrossSectionRadiator*>(clone.get());
  ASSERT_NE(from_xs, nullptr);
  EXPECT_EQ(from_xs->DensityProfileName(), "O3");
}

// ============================================================================
// FromCrossSectionRadiator UpdateState Tests
// ============================================================================

TEST_F(RadiatorTestFixture, UpdateState)
{
  auto xs = MakeSimpleCrossSection("O3", 1e-18);
  FromCrossSectionRadiator radiator("O3", std::move(xs), "O3");

  radiator.UpdateState(grids_, profiles_);

  EXPECT_TRUE(radiator.HasState());

  const auto& state = radiator.State();
  EXPECT_EQ(state.NumberOfLayers(), 3u);
  EXPECT_EQ(state.NumberOfWavelengths(), 3u);
}

TEST_F(RadiatorTestFixture, OpticalDepthCalculation)
{
  // Use a known cross-section value
  double sigma = 1e-18;  // cm^2
  auto xs = MakeSimpleCrossSection("O3", sigma);
  FromCrossSectionRadiator radiator("O3", std::move(xs), "O3");

  radiator.UpdateState(grids_, profiles_);

  const auto& state = radiator.State();

  // Layer 0: N = 1e12 molecules/cm^3, Δz = 10 km = 1e6 cm
  // τ = σ × N × Δz = 1e-18 × 1e12 × 1e6 = 1.0
  // Note: altitude grid deltas are in km, converted to cm in radiator
  double expected_tau_0 = sigma * 1e12 * 10.0 * 1e5;
  EXPECT_NEAR(state.optical_depth[0][0], expected_tau_0, expected_tau_0 * 1e-6);

  // Layer 1: N = 5e12
  double expected_tau_1 = sigma * 5e12 * 10.0 * 1e5;
  EXPECT_NEAR(state.optical_depth[1][0], expected_tau_1, expected_tau_1 * 1e-6);

  // Layer 2: N = 1e13
  double expected_tau_2 = sigma * 1e13 * 10.0 * 1e5;
  EXPECT_NEAR(state.optical_depth[2][0], expected_tau_2, expected_tau_2 * 1e-6);
}

TEST_F(RadiatorTestFixture, PureAbsorberProperties)
{
  auto xs = MakeSimpleCrossSection("O3", 1e-18);
  FromCrossSectionRadiator radiator("O3", std::move(xs), "O3");

  radiator.UpdateState(grids_, profiles_);

  const auto& state = radiator.State();

  // Pure absorber: ω = 0, g = 0
  for (std::size_t i = 0; i < state.NumberOfLayers(); ++i)
  {
    for (std::size_t j = 0; j < state.NumberOfWavelengths(); ++j)
    {
      EXPECT_DOUBLE_EQ(state.single_scattering_albedo[i][j], 0.0);
      EXPECT_DOUBLE_EQ(state.asymmetry_factor[i][j], 0.0);
    }
  }
}

TEST_F(RadiatorTestFixture, TotalOpticalDepth)
{
  double sigma = 1e-18;
  auto xs = MakeSimpleCrossSection("O3", sigma);
  FromCrossSectionRadiator radiator("O3", std::move(xs), "O3");

  radiator.UpdateState(grids_, profiles_);

  auto total_tau = radiator.State().TotalOpticalDepth();

  ASSERT_EQ(total_tau.size(), 3u);

  // Total = sum of all layers
  // N values: 1e12, 5e12, 1e13; Δz = 10 km = 1e6 cm each
  double expected_total = sigma * (1e12 + 5e12 + 1e13) * 10.0 * 1e5;
  EXPECT_NEAR(total_tau[0], expected_total, expected_total * 1e-6);
}

// ============================================================================
// FromCrossSectionRadiator Multiple Updates
// ============================================================================

TEST_F(RadiatorTestFixture, UpdateStateMultipleTimes)
{
  auto xs = MakeSimpleCrossSection("O3", 1e-18);
  FromCrossSectionRadiator radiator("O3", std::move(xs), "O3");

  radiator.UpdateState(grids_, profiles_);
  auto first_tau = radiator.State().optical_depth[0][0];

  // Update again - should give same result
  radiator.UpdateState(grids_, profiles_);
  auto second_tau = radiator.State().optical_depth[0][0];

  EXPECT_DOUBLE_EQ(first_tau, second_tau);
}

// ============================================================================
// Radiator State Access Tests
// ============================================================================

TEST_F(RadiatorTestFixture, StateAccessBeforeUpdate)
{
  auto xs = MakeSimpleCrossSection("O3", 1e-18);
  FromCrossSectionRadiator radiator("O3", std::move(xs), "O3");

  // State should be empty before update
  EXPECT_TRUE(radiator.State().Empty());
  EXPECT_FALSE(radiator.HasState());
}

TEST_F(RadiatorTestFixture, GetCrossSection)
{
  auto xs = MakeSimpleCrossSection("O3", 1e-18);
  FromCrossSectionRadiator radiator("O3", std::move(xs), "O3");

  const auto& retrieved_xs = radiator.GetCrossSection();
  EXPECT_EQ(retrieved_xs.Name(), "O3");
}

// ============================================================================
// Physical Scenario Tests
// ============================================================================

TEST_F(RadiatorTestFixture, RealisticO3OpticalDepth)
{
  // Create a wavelength grid that matches the cross-section range
  // Use edges that give midpoints at 280, 300, 320 nm
  GridSpec wl_spec{ "wavelength", "nm", 3 };
  std::vector<double> wl_edges = { 270.0, 290.0, 310.0, 330.0 };
  GridWarehouse local_grids;
  local_grids.Add(Grid(wl_spec, wl_edges));

  // Add altitude grid from fixture
  GridSpec alt_spec{ "altitude", "km", 3 };
  std::vector<double> alt_edges = { 0.0, 10.0, 20.0, 30.0 };
  local_grids.Add(Grid(alt_spec, alt_edges));

  // Use realistic O3 cross-section at 300 nm (~1e-19 cm^2)
  std::vector<double> wavelengths = { 280.0, 300.0, 320.0 };
  std::vector<double> xs_values = { 5e-19, 1e-19, 2e-20 };  // Decreasing with wavelength
  auto xs = std::make_unique<BaseCrossSection>("O3", wavelengths, xs_values);

  FromCrossSectionRadiator radiator("O3", std::move(xs), "O3");
  radiator.UpdateState(local_grids, profiles_);

  const auto& state = radiator.State();

  // Optical depth should be larger at shorter wavelengths
  EXPECT_GT(state.optical_depth[0][0], state.optical_depth[0][1]);
  EXPECT_GT(state.optical_depth[0][1], state.optical_depth[0][2]);

  // Optical depth should increase with altitude (O3 peaks in stratosphere)
  EXPECT_LT(state.optical_depth[0][0], state.optical_depth[1][0]);
  EXPECT_LT(state.optical_depth[1][0], state.optical_depth[2][0]);
}
