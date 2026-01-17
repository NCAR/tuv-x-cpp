#include <tuvx/grid/grid.hpp>
#include <tuvx/profile/profile.hpp>
#include <tuvx/profile/profile_spec.hpp>

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// ============================================================================
// ProfileSpec Tests
// ============================================================================

TEST(ProfileSpecTest, DefaultConstruction)
{
  ProfileSpec spec;
  EXPECT_TRUE(spec.name.empty());
  EXPECT_TRUE(spec.units.empty());
  EXPECT_EQ(spec.n_cells, 0u);
  EXPECT_DOUBLE_EQ(spec.scale_height, 8.0e3);
}

TEST(ProfileSpecTest, DesignatedInitializers)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 50, .scale_height = 7000.0 };
  EXPECT_EQ(spec.name, "temperature");
  EXPECT_EQ(spec.units, "K");
  EXPECT_EQ(spec.n_cells, 50u);
  EXPECT_DOUBLE_EQ(spec.scale_height, 7000.0);
}

TEST(ProfileSpecTest, Equality)
{
  ProfileSpec a{ .name = "temperature", .units = "K", .n_cells = 50 };
  ProfileSpec b{ .name = "temperature", .units = "K", .n_cells = 50 };
  ProfileSpec c{ .name = "pressure", .units = "K", .n_cells = 50 };

  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
}

TEST(ProfileSpecTest, Key)
{
  ProfileSpec spec{ .name = "air_density", .units = "molecules/cm^3", .n_cells = 80 };
  EXPECT_EQ(spec.Key(), "air_density|molecules/cm^3");
}

// ============================================================================
// Profile Construction Tests
// ============================================================================

TEST(ProfileTest, ConstructFromMidValues)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 4 };
  std::vector<double> mid_values = { 288.0, 270.0, 250.0, 220.0 };

  Profile profile(spec, mid_values);

  EXPECT_EQ(profile.Name(), "temperature");
  EXPECT_EQ(profile.Units(), "K");
  EXPECT_EQ(profile.NumberOfCells(), 4u);
  EXPECT_EQ(profile.MidValues().size(), 4u);
  EXPECT_EQ(profile.EdgeValues().size(), 5u);
}

TEST(ProfileTest, ConstructFromEdgeValues)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 3 };
  std::vector<double> edge_values = { 290.0, 280.0, 260.0, 230.0 };

  Profile profile(spec, edge_values, Profile::FromEdges);

  EXPECT_EQ(profile.NumberOfCells(), 3u);
  EXPECT_EQ(profile.EdgeValues().size(), 4u);
  EXPECT_EQ(profile.MidValues().size(), 3u);
}

TEST(ProfileTest, MidValuesMismatchThrows)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 4 };
  std::vector<double> mid_values = { 288.0, 270.0 };  // Wrong size

  EXPECT_THROW(Profile(spec, mid_values), TuvxInternalException);
}

TEST(ProfileTest, EdgeValuesMismatchThrows)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 3 };
  std::vector<double> edge_values = { 290.0, 280.0 };  // Wrong size

  EXPECT_THROW(Profile(spec, edge_values, Profile::FromEdges), TuvxInternalException);
}

// ============================================================================
// Profile Edge/Mid Interpolation Tests
// ============================================================================

TEST(ProfileTest, EdgeValuesFromMidpoints)
{
  ProfileSpec spec{ .name = "test", .units = "m", .n_cells = 3 };
  std::vector<double> mid_values = { 10.0, 20.0, 30.0 };

  Profile profile(spec, mid_values);
  auto edge_values = profile.EdgeValues();

  // First edge extrapolated: 1.5*10 - 0.5*20 = 5
  EXPECT_NEAR(edge_values[0], 5.0, 1e-10);

  // Internal edges averaged
  EXPECT_NEAR(edge_values[1], 15.0, 1e-10);  // (10+20)/2
  EXPECT_NEAR(edge_values[2], 25.0, 1e-10);  // (20+30)/2

  // Last edge extrapolated: 1.5*30 - 0.5*20 = 35
  EXPECT_NEAR(edge_values[3], 35.0, 1e-10);
}

TEST(ProfileTest, MidValuesFromEdges)
{
  ProfileSpec spec{ .name = "test", .units = "m", .n_cells = 3 };
  std::vector<double> edge_values = { 0.0, 10.0, 20.0, 30.0 };

  Profile profile(spec, edge_values, Profile::FromEdges);
  auto mid_values = profile.MidValues();

  // Midpoints are averages of edges
  EXPECT_NEAR(mid_values[0], 5.0, 1e-10);   // (0+10)/2
  EXPECT_NEAR(mid_values[1], 15.0, 1e-10);  // (10+20)/2
  EXPECT_NEAR(mid_values[2], 25.0, 1e-10);  // (20+30)/2
}

TEST(ProfileTest, SingleCellProfile)
{
  ProfileSpec spec{ .name = "test", .units = "m", .n_cells = 1 };
  std::vector<double> mid_values = { 100.0 };

  Profile profile(spec, mid_values);
  auto edge_values = profile.EdgeValues();

  EXPECT_EQ(edge_values.size(), 2u);
  EXPECT_NEAR(edge_values[0], 100.0, 1e-10);
  EXPECT_NEAR(edge_values[1], 100.0, 1e-10);
}

// ============================================================================
// Profile Layer Density Tests
// ============================================================================

TEST(ProfileTest, LayerDensityCalculation)
{
  // Set up altitude grid (km)
  GridSpec grid_spec{ .name = "altitude", .units = "km", .n_cells = 3 };
  Grid grid = Grid::EquallySpaced(grid_spec, 0.0, 30.0);  // 10 km layers

  // Air density profile (arbitrary units)
  ProfileSpec profile_spec{ .name = "air_density", .units = "units/km^3", .n_cells = 3 };
  std::vector<double> densities = { 100.0, 50.0, 25.0 };  // Decreasing with altitude

  Profile profile(profile_spec, densities);
  profile.CalculateLayerDensities(grid);

  EXPECT_TRUE(profile.HasLayerDensities());
  auto layer_dens = profile.LayerDensities();

  // layer_dens = mid_value * delta
  EXPECT_NEAR(layer_dens[0], 100.0 * 10.0, 1e-10);  // 1000
  EXPECT_NEAR(layer_dens[1], 50.0 * 10.0, 1e-10);   // 500
  EXPECT_NEAR(layer_dens[2], 25.0 * 10.0, 1e-10);   // 250
}

TEST(ProfileTest, LayerDensityGridMismatchThrows)
{
  GridSpec grid_spec{ .name = "altitude", .units = "km", .n_cells = 5 };
  Grid grid = Grid::EquallySpaced(grid_spec, 0.0, 50.0);

  ProfileSpec profile_spec{ .name = "density", .units = "units", .n_cells = 3 };
  std::vector<double> values = { 1.0, 2.0, 3.0 };

  Profile profile(profile_spec, values);
  EXPECT_THROW(profile.CalculateLayerDensities(grid), TuvxInternalException);
}

// ============================================================================
// Profile Burden Density Tests
// ============================================================================

TEST(ProfileTest, BurdenCalculation)
{
  // Set up altitude grid
  GridSpec grid_spec{ .name = "altitude", .units = "km", .n_cells = 4 };
  Grid grid = Grid::EquallySpaced(grid_spec, 0.0, 40.0);  // 10 km layers

  // Density profile
  ProfileSpec profile_spec{ .name = "density", .units = "units", .n_cells = 4 };
  std::vector<double> densities = { 10.0, 8.0, 6.0, 4.0 };

  Profile profile(profile_spec, densities);
  profile.CalculateBurden(grid);

  EXPECT_TRUE(profile.HasBurdenDensities());
  auto burden = profile.BurdenDensities();

  // Layer densities: [100, 80, 60, 40]
  // Burden integrates from top down:
  // burden[4] = 0
  // burden[3] = 0 + 40 = 40
  // burden[2] = 40 + 60 = 100
  // burden[1] = 100 + 80 = 180
  // burden[0] = 180 + 100 = 280

  EXPECT_NEAR(burden[4], 0.0, 1e-10);
  EXPECT_NEAR(burden[3], 40.0, 1e-10);
  EXPECT_NEAR(burden[2], 100.0, 1e-10);
  EXPECT_NEAR(burden[1], 180.0, 1e-10);
  EXPECT_NEAR(burden[0], 280.0, 1e-10);
}

TEST(ProfileTest, BurdenTriggersLayerDensityCalculation)
{
  GridSpec grid_spec{ .name = "altitude", .units = "km", .n_cells = 3 };
  Grid grid = Grid::EquallySpaced(grid_spec, 0.0, 30.0);

  ProfileSpec profile_spec{ .name = "density", .units = "units", .n_cells = 3 };
  std::vector<double> values = { 1.0, 2.0, 3.0 };

  Profile profile(profile_spec, values);

  EXPECT_FALSE(profile.HasLayerDensities());
  profile.CalculateBurden(grid);
  EXPECT_TRUE(profile.HasLayerDensities());
  EXPECT_TRUE(profile.HasBurdenDensities());
}

// ============================================================================
// Profile Extrapolation Tests
// ============================================================================

TEST(ProfileTest, ExtrapolateAbove)
{
  ProfileSpec spec{ .name = "density", .units = "units", .n_cells = 3, .scale_height = 1000.0 };
  std::vector<double> values = { 1000.0, 500.0, 100.0 };  // Top value is 100

  Profile profile(spec, values);

  // At grid top, should return top value
  EXPECT_NEAR(profile.ExtrapolateAbove(30.0, 30.0), 100.0, 1e-10);

  // One scale height above, should decay by factor of e
  double one_scale_height_above = profile.ExtrapolateAbove(31000.0, 30000.0);
  EXPECT_NEAR(one_scale_height_above, 100.0 * std::exp(-1.0), 1e-8);

  // Two scale heights above
  double two_scale_heights_above = profile.ExtrapolateAbove(32000.0, 30000.0);
  EXPECT_NEAR(two_scale_heights_above, 100.0 * std::exp(-2.0), 1e-8);
}

TEST(ProfileTest, ExtrapolateAtOrBelowTop)
{
  ProfileSpec spec{ .name = "density", .units = "units", .n_cells = 3, .scale_height = 8000.0 };
  std::vector<double> values = { 100.0, 50.0, 25.0 };

  Profile profile(spec, values);

  // At or below grid top, just return top value
  EXPECT_NEAR(profile.ExtrapolateAbove(30.0, 30.0), 25.0, 1e-10);
  EXPECT_NEAR(profile.ExtrapolateAbove(20.0, 30.0), 25.0, 1e-10);
}

// ============================================================================
// Profile Practical Example Tests
// ============================================================================

TEST(ProfileTest, AtmosphericDensityProfile)
{
  // Typical atmospheric setup
  GridSpec grid_spec{ .name = "altitude", .units = "km", .n_cells = 5 };
  Grid grid = Grid::EquallySpaced(grid_spec, 0.0, 50.0);  // 0-50 km, 10 km layers

  // Exponentially decreasing density (normalized)
  ProfileSpec profile_spec{ .name = "air_density", .units = "molecules/cm^3", .n_cells = 5, .scale_height = 8000.0 };
  std::vector<double> densities;
  double scale_height_km = 8.0;
  for (std::size_t i = 0; i < 5; ++i)
  {
    double alt = 5.0 + i * 10.0;  // Midpoint altitudes: 5, 15, 25, 35, 45 km
    densities.push_back(2.5e19 * std::exp(-alt / scale_height_km));
  }

  Profile profile(profile_spec, densities);
  profile.CalculateBurden(grid);

  // Verify profile captures exponential decrease
  auto burden = profile.BurdenDensities();
  EXPECT_GT(burden[0], burden[1]);  // Total column > column above first layer
  EXPECT_GT(burden[1], burden[2]);
  EXPECT_GT(burden[4], 0.0);        // Some density above 40 km
  EXPECT_DOUBLE_EQ(burden[5], 0.0);  // Nothing above top
}

TEST(ProfileTest, OzoneProfile)
{
  // Ozone layer peaks in stratosphere
  GridSpec grid_spec{ .name = "altitude", .units = "km", .n_cells = 6 };
  Grid grid = Grid::EquallySpaced(grid_spec, 0.0, 60.0);

  ProfileSpec profile_spec{ .name = "ozone", .units = "molecules/cm^3", .n_cells = 6 };
  // Ozone peaks around 20-25 km
  std::vector<double> ozone = {
    1e12,   // 0-10 km (troposphere, low)
    3e12,   // 10-20 km (lower stratosphere)
    5e12,   // 20-30 km (ozone max)
    4e12,   // 30-40 km (upper stratosphere)
    1e12,   // 40-50 km (declining)
    0.5e12  // 50-60 km (mesosphere)
  };

  Profile profile(profile_spec, ozone);
  profile.CalculateLayerDensities(grid);

  auto layer_dens = profile.LayerDensities();

  // Check that layer 2 (20-30 km) has highest integrated ozone
  EXPECT_GT(layer_dens[2], layer_dens[0]);
  EXPECT_GT(layer_dens[2], layer_dens[1]);
  EXPECT_GT(layer_dens[2], layer_dens[3]);
}
