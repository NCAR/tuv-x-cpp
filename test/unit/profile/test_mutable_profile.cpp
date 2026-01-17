#include <tuvx/grid/grid.hpp>
#include <tuvx/profile/mutable_profile.hpp>

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// ============================================================================
// MutableProfile Construction Tests
// ============================================================================

TEST(MutableProfileTest, ConstructFromMidValues)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 4 };
  std::vector<double> mid_values = { 288.0, 270.0, 250.0, 220.0 };

  MutableProfile profile(spec, mid_values);

  EXPECT_EQ(profile.Name(), "temperature");
  EXPECT_EQ(profile.Units(), "K");
  EXPECT_EQ(profile.NumberOfCells(), 4u);
  EXPECT_EQ(profile.MidValues().size(), 4u);
  EXPECT_EQ(profile.EdgeValues().size(), 5u);
}

TEST(MutableProfileTest, ConstructFromEdgeValues)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 3 };
  std::vector<double> edge_values = { 290.0, 280.0, 260.0, 230.0 };

  MutableProfile profile(spec, edge_values, Profile::FromEdges);

  EXPECT_EQ(profile.NumberOfCells(), 3u);
  EXPECT_EQ(profile.MidValues().size(), 3u);
  EXPECT_EQ(profile.EdgeValues().size(), 4u);
}

TEST(MutableProfileTest, ConstructFromProfile)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 3 };
  std::vector<double> mid_values = { 288.0, 260.0, 230.0 };

  Profile immutable_profile(spec, mid_values);
  MutableProfile mutable_profile(immutable_profile);

  EXPECT_EQ(mutable_profile.Name(), "temperature");
  EXPECT_EQ(mutable_profile.NumberOfCells(), 3u);

  auto mutable_mids = mutable_profile.MidValues();
  auto immutable_mids = immutable_profile.MidValues();
  for (std::size_t i = 0; i < 3; ++i)
  {
    EXPECT_DOUBLE_EQ(mutable_mids[i], immutable_mids[i]);
  }
}

TEST(MutableProfileTest, MidValuesMismatchThrows)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 4 };
  std::vector<double> mid_values = { 288.0 };  // Wrong size

  EXPECT_THROW(MutableProfile(spec, mid_values), TuvxInternalException);
}

// ============================================================================
// MutableProfile Modification Tests
// ============================================================================

TEST(MutableProfileTest, SetMidValues)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 3 };
  std::vector<double> initial = { 288.0, 270.0, 250.0 };

  MutableProfile profile(spec, initial);

  std::vector<double> new_values = { 300.0, 280.0, 260.0 };
  profile.SetMidValues(new_values);

  auto mid_values = profile.MidValues();
  EXPECT_DOUBLE_EQ(mid_values[0], 300.0);
  EXPECT_DOUBLE_EQ(mid_values[1], 280.0);
  EXPECT_DOUBLE_EQ(mid_values[2], 260.0);

  // Edge values should be recomputed
  auto edge_values = profile.EdgeValues();
  EXPECT_NEAR(edge_values[1], 290.0, 1e-10);  // (300+280)/2
  EXPECT_NEAR(edge_values[2], 270.0, 1e-10);  // (280+260)/2
}

TEST(MutableProfileTest, SetMidValuesWrongSize)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 3 };
  std::vector<double> initial = { 288.0, 270.0, 250.0 };

  MutableProfile profile(spec, initial);

  std::vector<double> wrong_size = { 300.0 };
  EXPECT_THROW(profile.SetMidValues(wrong_size), TuvxInternalException);
}

TEST(MutableProfileTest, SetSingleMidValue)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 3 };
  std::vector<double> initial = { 288.0, 270.0, 250.0 };

  MutableProfile profile(spec, initial);

  profile.SetMidValue(1, 280.0);
  profile.Update();

  auto mid_values = profile.MidValues();
  EXPECT_DOUBLE_EQ(mid_values[1], 280.0);

  // Edge values should be recomputed
  auto edge_values = profile.EdgeValues();
  EXPECT_NEAR(edge_values[1], 284.0, 1e-10);  // (288+280)/2
  EXPECT_NEAR(edge_values[2], 265.0, 1e-10);  // (280+250)/2
}

TEST(MutableProfileTest, SetSingleMidValueOutOfRange)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 3 };
  std::vector<double> initial = { 288.0, 270.0, 250.0 };

  MutableProfile profile(spec, initial);

  EXPECT_THROW(profile.SetMidValue(10, 300.0), TuvxInternalException);
}

TEST(MutableProfileTest, MutableMidValuesAccess)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 3 };
  std::vector<double> initial = { 288.0, 270.0, 250.0 };

  MutableProfile profile(spec, initial);

  auto mutable_mids = profile.MutableMidValues();
  mutable_mids[0] = 300.0;
  mutable_mids[2] = 240.0;
  profile.Update();

  auto mid_values = profile.MidValues();
  EXPECT_DOUBLE_EQ(mid_values[0], 300.0);
  EXPECT_DOUBLE_EQ(mid_values[2], 240.0);
}

TEST(MutableProfileTest, SetEdgeValues)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 3 };
  std::vector<double> initial = { 288.0, 270.0, 250.0 };

  MutableProfile profile(spec, initial);

  std::vector<double> new_edges = { 300.0, 280.0, 260.0, 240.0 };
  profile.SetEdgeValues(new_edges);

  auto edge_values = profile.EdgeValues();
  EXPECT_DOUBLE_EQ(edge_values[0], 300.0);
  EXPECT_DOUBLE_EQ(edge_values[3], 240.0);

  // Mid values should be recomputed as averages
  auto mid_values = profile.MidValues();
  EXPECT_NEAR(mid_values[0], 290.0, 1e-10);  // (300+280)/2
  EXPECT_NEAR(mid_values[1], 270.0, 1e-10);  // (280+260)/2
  EXPECT_NEAR(mid_values[2], 250.0, 1e-10);  // (260+240)/2
}

TEST(MutableProfileTest, UpdateFromEdges)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 3 };
  std::vector<double> initial_mids = { 288.0, 270.0, 250.0 };

  MutableProfile profile(spec, initial_mids);

  auto mutable_edges = profile.MutableEdgeValues();
  mutable_edges[0] = 300.0;
  mutable_edges[3] = 230.0;
  profile.UpdateFromEdges();

  // Mid values should be recalculated from edges
  auto mid_values = profile.MidValues();
  // Original edge[1] and edge[2] were computed from initial mids
  // After changing edge[0] and edge[3], mid values recalculated
  EXPECT_EQ(mid_values.size(), 3u);
}

// ============================================================================
// MutableProfile Derived Quantities Tests
// ============================================================================

TEST(MutableProfileTest, ModificationInvalidatesDerivedQuantities)
{
  GridSpec grid_spec{ .name = "altitude", .units = "km", .n_cells = 3 };
  Grid grid = Grid::EquallySpaced(grid_spec, 0.0, 30.0);

  ProfileSpec profile_spec{ .name = "density", .units = "units", .n_cells = 3 };
  std::vector<double> values = { 100.0, 50.0, 25.0 };

  MutableProfile profile(profile_spec, values);
  profile.CalculateBurden(grid);

  EXPECT_TRUE(profile.HasLayerDensities());
  EXPECT_TRUE(profile.HasBurdenDensities());

  // Modify values
  profile.SetMidValue(0, 200.0);
  profile.Update();

  // Derived quantities should be invalidated
  EXPECT_FALSE(profile.HasLayerDensities());
  EXPECT_FALSE(profile.HasBurdenDensities());
}

TEST(MutableProfileTest, RecalculateAfterModification)
{
  GridSpec grid_spec{ .name = "altitude", .units = "km", .n_cells = 3 };
  Grid grid = Grid::EquallySpaced(grid_spec, 0.0, 30.0);

  ProfileSpec profile_spec{ .name = "density", .units = "units", .n_cells = 3 };
  std::vector<double> values = { 100.0, 50.0, 25.0 };

  MutableProfile profile(profile_spec, values);
  profile.CalculateLayerDensities(grid);

  auto initial_layer_dens = profile.LayerDensities();
  EXPECT_NEAR(initial_layer_dens[0], 1000.0, 1e-10);  // 100 * 10

  // Modify and recalculate
  profile.SetMidValue(0, 200.0);
  profile.Update();
  profile.CalculateLayerDensities(grid);

  auto new_layer_dens = profile.LayerDensities();
  EXPECT_NEAR(new_layer_dens[0], 2000.0, 1e-10);  // 200 * 10
}

// ============================================================================
// MutableProfile ToProfile Conversion Tests
// ============================================================================

TEST(MutableProfileTest, ToProfile)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 3 };
  std::vector<double> values = { 288.0, 270.0, 250.0 };

  MutableProfile mutable_profile(spec, values);
  Profile immutable_profile = mutable_profile.ToProfile();

  EXPECT_EQ(immutable_profile.Name(), "temperature");
  EXPECT_EQ(immutable_profile.NumberOfCells(), 3u);

  auto mutable_mids = mutable_profile.MidValues();
  auto immutable_mids = immutable_profile.MidValues();
  for (std::size_t i = 0; i < 3; ++i)
  {
    EXPECT_DOUBLE_EQ(mutable_mids[i], immutable_mids[i]);
  }
}

TEST(MutableProfileTest, ToProfileAfterModification)
{
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 3 };
  std::vector<double> values = { 288.0, 270.0, 250.0 };

  MutableProfile mutable_profile(spec, values);
  mutable_profile.SetMidValue(0, 300.0);
  mutable_profile.Update();

  Profile immutable_profile = mutable_profile.ToProfile();

  auto mid_values = immutable_profile.MidValues();
  EXPECT_DOUBLE_EQ(mid_values[0], 300.0);
}

// ============================================================================
// MutableProfile Practical Usage Tests
// ============================================================================

TEST(MutableProfileTest, DiurnalTemperatureVariation)
{
  // Simulate updating temperature profile during day
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 4 };
  std::vector<double> morning_temps = { 288.0, 275.0, 260.0, 240.0 };

  MutableProfile temp_profile(spec, morning_temps);

  // Apply daytime heating in lower atmosphere
  auto mutable_temps = temp_profile.MutableMidValues();
  mutable_temps[0] += 10.0;  // Surface warms by 10K
  mutable_temps[1] += 5.0;   // Lower troposphere warms less
  temp_profile.Update();

  auto afternoon_temps = temp_profile.MidValues();
  EXPECT_NEAR(afternoon_temps[0], 298.0, 1e-10);
  EXPECT_NEAR(afternoon_temps[1], 280.0, 1e-10);
  EXPECT_NEAR(afternoon_temps[2], 260.0, 1e-10);  // Unchanged
  EXPECT_NEAR(afternoon_temps[3], 240.0, 1e-10);  // Unchanged
}

TEST(MutableProfileTest, AdvectedAirMass)
{
  // Simulate replacing entire profile with new air mass
  ProfileSpec spec{ .name = "ozone", .units = "molecules/cm^3", .n_cells = 3 };
  std::vector<double> initial = { 1e12, 5e12, 2e12 };

  MutableProfile profile(spec, initial);

  // Replace with polluted air mass
  std::vector<double> polluted = { 0.5e12, 3e12, 2e12 };
  profile.SetMidValues(polluted);

  auto new_values = profile.MidValues();
  EXPECT_NEAR(new_values[0], 0.5e12, 1e6);
  EXPECT_NEAR(new_values[1], 3e12, 1e6);
}
