#include <tuvx/cross_section/types/base.hpp>
#include <tuvx/grid/grid.hpp>
#include <tuvx/grid/grid_warehouse.hpp>
#include <tuvx/profile/profile.hpp>
#include <tuvx/profile/profile_warehouse.hpp>
#include <tuvx/radiator/radiator_warehouse.hpp>
#include <tuvx/radiator/types/from_cross_section.hpp>

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// Helper to create a test radiator
std::unique_ptr<Radiator> MakeTestRadiator(const std::string& name, double xs_value = 1e-18)
{
  std::vector<double> wavelengths = { 200.0, 300.0, 400.0 };
  std::vector<double> xs_values = { xs_value, xs_value, xs_value };
  auto xs = std::make_unique<BaseCrossSection>(name, wavelengths, xs_values);
  return std::make_unique<FromCrossSectionRadiator>(name, std::move(xs), name);
}

// Test fixture with grids and profiles
class RadiatorWarehouseTestFixture : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    // Wavelength grid
    GridSpec wl_spec{ "wavelength", "nm", 3 };
    std::vector<double> wl_edges = { 150.0, 250.0, 350.0, 450.0 };
    grids_.Add(Grid(wl_spec, wl_edges));

    // Altitude grid
    GridSpec alt_spec{ "altitude", "km", 2 };
    std::vector<double> alt_edges = { 0.0, 10.0, 20.0 };
    grids_.Add(Grid(alt_spec, alt_edges));

    // Temperature profile
    ProfileSpec temp_spec{ "temperature", "K", 2 };
    std::vector<double> temp_values = { 288.0, 250.0 };
    profiles_.Add(Profile(temp_spec, temp_values));

    // O3 profile
    ProfileSpec o3_spec{ "O3", "molecules/cm^3", 2 };
    std::vector<double> o3_values = { 1e12, 5e12 };
    profiles_.Add(Profile(o3_spec, o3_values));

    // NO2 profile
    ProfileSpec no2_spec{ "NO2", "molecules/cm^3", 2 };
    std::vector<double> no2_values = { 1e11, 2e11 };
    profiles_.Add(Profile(no2_spec, no2_values));
  }

  GridWarehouse grids_;
  ProfileWarehouse profiles_;
};

// ============================================================================
// RadiatorWarehouse Basic Tests
// ============================================================================

TEST(RadiatorWarehouseTest, DefaultConstruction)
{
  RadiatorWarehouse warehouse;

  EXPECT_TRUE(warehouse.Empty());
  EXPECT_EQ(warehouse.Size(), 0u);
}

TEST(RadiatorWarehouseTest, AddAndGetByName)
{
  RadiatorWarehouse warehouse;

  auto handle = warehouse.Add(MakeTestRadiator("O3"));

  EXPECT_TRUE(handle.IsValid());
  EXPECT_EQ(warehouse.Size(), 1u);
  EXPECT_FALSE(warehouse.Empty());

  const auto& radiator = warehouse.Get("O3");
  EXPECT_EQ(radiator.Name(), "O3");
}

TEST(RadiatorWarehouseTest, AddAndGetByHandle)
{
  RadiatorWarehouse warehouse;

  auto handle = warehouse.Add(MakeTestRadiator("O3"));

  const auto& radiator = warehouse.Get(handle);
  EXPECT_EQ(radiator.Name(), "O3");
}

TEST(RadiatorWarehouseTest, AddMultiple)
{
  RadiatorWarehouse warehouse;

  auto h1 = warehouse.Add(MakeTestRadiator("O3"));
  auto h2 = warehouse.Add(MakeTestRadiator("NO2"));
  auto h3 = warehouse.Add(MakeTestRadiator("HCHO"));

  EXPECT_EQ(warehouse.Size(), 3u);

  EXPECT_EQ(warehouse.Get(h1).Name(), "O3");
  EXPECT_EQ(warehouse.Get(h2).Name(), "NO2");
  EXPECT_EQ(warehouse.Get(h3).Name(), "HCHO");
}

TEST(RadiatorWarehouseTest, Exists)
{
  RadiatorWarehouse warehouse;
  warehouse.Add(MakeTestRadiator("O3"));

  EXPECT_TRUE(warehouse.Exists("O3"));
  EXPECT_FALSE(warehouse.Exists("NO2"));
}

TEST(RadiatorWarehouseTest, Names)
{
  RadiatorWarehouse warehouse;
  warehouse.Add(MakeTestRadiator("O3"));
  warehouse.Add(MakeTestRadiator("NO2"));

  auto names = warehouse.Names();

  ASSERT_EQ(names.size(), 2u);
  EXPECT_EQ(names[0], "O3");
  EXPECT_EQ(names[1], "NO2");
}

TEST(RadiatorWarehouseTest, Clear)
{
  RadiatorWarehouse warehouse;
  warehouse.Add(MakeTestRadiator("O3"));
  warehouse.Add(MakeTestRadiator("NO2"));

  warehouse.Clear();

  EXPECT_TRUE(warehouse.Empty());
  EXPECT_EQ(warehouse.Size(), 0u);
}

// ============================================================================
// RadiatorWarehouse Error Cases
// ============================================================================

TEST(RadiatorWarehouseTest, AddDuplicate)
{
  RadiatorWarehouse warehouse;
  warehouse.Add(MakeTestRadiator("O3"));

  EXPECT_THROW(warehouse.Add(MakeTestRadiator("O3")), std::runtime_error);
}

TEST(RadiatorWarehouseTest, AddNull)
{
  RadiatorWarehouse warehouse;

  EXPECT_THROW(warehouse.Add(nullptr), std::runtime_error);
}

TEST(RadiatorWarehouseTest, GetNonexistentByName)
{
  RadiatorWarehouse warehouse;

  EXPECT_THROW(warehouse.Get("O3"), std::out_of_range);
}

TEST(RadiatorWarehouseTest, GetNonexistentByHandle)
{
  RadiatorWarehouse warehouse;

  RadiatorHandle invalid;
  EXPECT_THROW(warehouse.Get(invalid), std::out_of_range);
}

// ============================================================================
// RadiatorWarehouse UpdateAll Tests
// ============================================================================

TEST_F(RadiatorWarehouseTestFixture, UpdateAll)
{
  RadiatorWarehouse warehouse;
  warehouse.Add(MakeTestRadiator("O3"));
  warehouse.Add(MakeTestRadiator("NO2"));

  // Before update
  EXPECT_FALSE(warehouse.Get("O3").HasState());
  EXPECT_FALSE(warehouse.Get("NO2").HasState());

  warehouse.UpdateAll(grids_, profiles_);

  // After update
  EXPECT_TRUE(warehouse.Get("O3").HasState());
  EXPECT_TRUE(warehouse.Get("NO2").HasState());
}

TEST_F(RadiatorWarehouseTestFixture, GetMutable)
{
  RadiatorWarehouse warehouse;
  warehouse.Add(MakeTestRadiator("O3"));

  // Update via mutable access
  warehouse.GetMutable("O3").UpdateState(grids_, profiles_);

  EXPECT_TRUE(warehouse.Get("O3").HasState());
}

// ============================================================================
// RadiatorWarehouse CombinedState Tests
// ============================================================================

TEST_F(RadiatorWarehouseTestFixture, CombinedStateEmpty)
{
  RadiatorWarehouse warehouse;

  auto combined = warehouse.CombinedState();

  EXPECT_TRUE(combined.Empty());
}

TEST_F(RadiatorWarehouseTestFixture, CombinedStateSingleRadiator)
{
  RadiatorWarehouse warehouse;
  warehouse.Add(MakeTestRadiator("O3"));
  warehouse.UpdateAll(grids_, profiles_);

  auto combined = warehouse.CombinedState();

  EXPECT_FALSE(combined.Empty());
  EXPECT_EQ(combined.NumberOfLayers(), 2u);
  EXPECT_EQ(combined.NumberOfWavelengths(), 3u);

  // Combined should equal single radiator's state
  const auto& o3_state = warehouse.Get("O3").State();
  EXPECT_DOUBLE_EQ(combined.optical_depth[0][0], o3_state.optical_depth[0][0]);
}

TEST_F(RadiatorWarehouseTestFixture, CombinedStateMultipleRadiators)
{
  RadiatorWarehouse warehouse;
  warehouse.Add(MakeTestRadiator("O3", 1e-18));
  warehouse.Add(MakeTestRadiator("NO2", 2e-18));
  warehouse.UpdateAll(grids_, profiles_);

  auto combined = warehouse.CombinedState();

  // Optical depths should add
  double o3_tau = warehouse.Get("O3").State().optical_depth[0][0];
  double no2_tau = warehouse.Get("NO2").State().optical_depth[0][0];

  EXPECT_NEAR(combined.optical_depth[0][0], o3_tau + no2_tau, 1e-20);
}

TEST_F(RadiatorWarehouseTestFixture, CombinedStateBeforeUpdate)
{
  RadiatorWarehouse warehouse;
  warehouse.Add(MakeTestRadiator("O3"));

  // States not updated yet
  auto combined = warehouse.CombinedState();

  // Should be empty since no radiators have state
  EXPECT_TRUE(combined.Empty());
}

// ============================================================================
// RadiatorHandle Tests
// ============================================================================

TEST(RadiatorHandleTest, DefaultInvalid)
{
  RadiatorHandle handle;
  EXPECT_FALSE(handle.IsValid());
}

TEST(RadiatorHandleTest, ValidAfterAdd)
{
  RadiatorWarehouse warehouse;
  auto handle = warehouse.Add(MakeTestRadiator("O3"));

  EXPECT_TRUE(handle.IsValid());
  EXPECT_EQ(handle.Index(), 0u);
}

// ============================================================================
// RadiatorWarehouse Move Semantics
// ============================================================================

TEST(RadiatorWarehouseTest, MoveConstruction)
{
  RadiatorWarehouse warehouse;
  warehouse.Add(MakeTestRadiator("O3"));
  warehouse.Add(MakeTestRadiator("NO2"));

  RadiatorWarehouse moved(std::move(warehouse));

  EXPECT_EQ(moved.Size(), 2u);
  EXPECT_EQ(moved.Get("O3").Name(), "O3");
}
