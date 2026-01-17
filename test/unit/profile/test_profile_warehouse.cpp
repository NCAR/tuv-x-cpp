#include <tuvx/profile/profile_warehouse.hpp>

#include <gtest/gtest.h>

using namespace tuvx;

// ============================================================================
// ProfileHandle Tests
// ============================================================================

TEST(ProfileHandleTest, DefaultIsInvalid)
{
  ProfileHandle handle;
  EXPECT_FALSE(handle.IsValid());
}

TEST(ProfileHandleTest, EqualityComparison)
{
  ProfileHandle h1;
  ProfileHandle h2;
  EXPECT_EQ(h1, h2);  // Both invalid
}

// ============================================================================
// ProfileWarehouse Basic Tests
// ============================================================================

TEST(ProfileWarehouseTest, InitiallyEmpty)
{
  ProfileWarehouse warehouse;
  EXPECT_TRUE(warehouse.Empty());
  EXPECT_EQ(warehouse.Size(), 0u);
}

TEST(ProfileWarehouseTest, AddAndGet)
{
  ProfileWarehouse warehouse;
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 10 };
  std::vector<double> values(10, 250.0);
  Profile profile(spec, values);

  ProfileHandle handle = warehouse.Add(std::move(profile));

  EXPECT_TRUE(handle.IsValid());
  EXPECT_EQ(warehouse.Size(), 1u);
  EXPECT_FALSE(warehouse.Empty());

  const Profile& retrieved = warehouse.Get(handle);
  EXPECT_EQ(retrieved.Name(), "temperature");
  EXPECT_EQ(retrieved.Units(), "K");
  EXPECT_EQ(retrieved.NumberOfCells(), 10u);
}

TEST(ProfileWarehouseTest, GetByNameAndUnits)
{
  ProfileWarehouse warehouse;
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 10 };
  std::vector<double> values(10, 250.0);
  Profile profile(spec, values);

  warehouse.Add(std::move(profile));

  const Profile& retrieved = warehouse.Get("temperature", "K");
  EXPECT_EQ(retrieved.Name(), "temperature");
}

TEST(ProfileWarehouseTest, ExistsCheck)
{
  ProfileWarehouse warehouse;
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 10 };
  std::vector<double> values(10, 250.0);
  Profile profile(spec, values);

  warehouse.Add(std::move(profile));

  EXPECT_TRUE(warehouse.Exists("temperature", "K"));
  EXPECT_FALSE(warehouse.Exists("temperature", "C"));
  EXPECT_FALSE(warehouse.Exists("pressure", "K"));
}

TEST(ProfileWarehouseTest, GetHandle)
{
  ProfileWarehouse warehouse;
  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 10 };
  std::vector<double> values(10, 250.0);
  Profile profile(spec, values);

  ProfileHandle add_handle = warehouse.Add(std::move(profile));
  ProfileHandle lookup_handle = warehouse.GetHandle("temperature", "K");

  EXPECT_TRUE(lookup_handle.IsValid());
  EXPECT_EQ(add_handle, lookup_handle);
}

TEST(ProfileWarehouseTest, GetHandleNotFound)
{
  ProfileWarehouse warehouse;
  ProfileHandle handle = warehouse.GetHandle("nonexistent", "units");
  EXPECT_FALSE(handle.IsValid());
}

// ============================================================================
// ProfileWarehouse Multiple Profiles Tests
// ============================================================================

TEST(ProfileWarehouseTest, MultipleProfiles)
{
  ProfileWarehouse warehouse;

  ProfileSpec temp_spec{ .name = "temperature", .units = "K", .n_cells = 50 };
  std::vector<double> temp_values(50, 250.0);
  ProfileHandle temp_handle = warehouse.Add(Profile(temp_spec, temp_values));

  ProfileSpec dens_spec{ .name = "air_density", .units = "molecules/cm^3", .n_cells = 50 };
  std::vector<double> dens_values(50, 2.5e19);
  ProfileHandle dens_handle = warehouse.Add(Profile(dens_spec, dens_values));

  ProfileSpec oz_spec{ .name = "ozone", .units = "molecules/cm^3", .n_cells = 50 };
  std::vector<double> oz_values(50, 5e12);
  ProfileHandle oz_handle = warehouse.Add(Profile(oz_spec, oz_values));

  EXPECT_EQ(warehouse.Size(), 3u);

  EXPECT_EQ(warehouse.Get(temp_handle).Name(), "temperature");
  EXPECT_EQ(warehouse.Get(dens_handle).Name(), "air_density");
  EXPECT_EQ(warehouse.Get(oz_handle).Name(), "ozone");

  EXPECT_EQ(warehouse.Get("temperature", "K").NumberOfCells(), 50u);
  EXPECT_EQ(warehouse.Get("air_density", "molecules/cm^3").NumberOfCells(), 50u);
  EXPECT_EQ(warehouse.Get("ozone", "molecules/cm^3").NumberOfCells(), 50u);
}

TEST(ProfileWarehouseTest, SameNameDifferentUnits)
{
  ProfileWarehouse warehouse;

  ProfileSpec k_spec{ .name = "temperature", .units = "K", .n_cells = 10 };
  std::vector<double> k_values(10, 250.0);
  warehouse.Add(Profile(k_spec, k_values));

  ProfileSpec c_spec{ .name = "temperature", .units = "C", .n_cells = 10 };
  std::vector<double> c_values(10, -23.0);
  warehouse.Add(Profile(c_spec, c_values));

  EXPECT_EQ(warehouse.Size(), 2u);
  EXPECT_EQ(warehouse.Get("temperature", "K").NumberOfCells(), 10u);
  EXPECT_EQ(warehouse.Get("temperature", "C").NumberOfCells(), 10u);
}

// ============================================================================
// ProfileWarehouse Error Handling Tests
// ============================================================================

TEST(ProfileWarehouseTest, DuplicateRejection)
{
  ProfileWarehouse warehouse;

  ProfileSpec spec{ .name = "temperature", .units = "K", .n_cells = 10 };
  std::vector<double> values1(10, 250.0);
  warehouse.Add(Profile(spec, values1));

  std::vector<double> values2(10, 300.0);
  EXPECT_THROW(warehouse.Add(Profile(spec, values2)), TuvxInternalException);
}

TEST(ProfileWarehouseTest, GetNotFoundThrows)
{
  ProfileWarehouse warehouse;
  EXPECT_THROW(warehouse.Get("nonexistent", "units"), TuvxInternalException);
}

TEST(ProfileWarehouseTest, GetInvalidHandleThrows)
{
  ProfileWarehouse warehouse;
  ProfileHandle invalid_handle;
  EXPECT_THROW(warehouse.Get(invalid_handle), TuvxInternalException);
}

// ============================================================================
// ProfileWarehouse Keys Tests
// ============================================================================

TEST(ProfileWarehouseTest, Keys)
{
  ProfileWarehouse warehouse;

  ProfileSpec temp_spec{ .name = "temperature", .units = "K", .n_cells = 10 };
  std::vector<double> temp_values(10, 250.0);
  warehouse.Add(Profile(temp_spec, temp_values));

  ProfileSpec dens_spec{ .name = "air_density", .units = "molecules/cm^3", .n_cells = 10 };
  std::vector<double> dens_values(10, 2.5e19);
  warehouse.Add(Profile(dens_spec, dens_values));

  auto keys = warehouse.Keys();
  EXPECT_EQ(keys.size(), 2u);

  // Keys should be in insertion order
  EXPECT_EQ(keys[0], "temperature|K");
  EXPECT_EQ(keys[1], "air_density|molecules/cm^3");
}

// ============================================================================
// ProfileWarehouse Clear Tests
// ============================================================================

TEST(ProfileWarehouseTest, Clear)
{
  ProfileWarehouse warehouse;

  ProfileSpec spec1{ .name = "temperature", .units = "K", .n_cells = 10 };
  std::vector<double> values1(10, 250.0);
  warehouse.Add(Profile(spec1, values1));

  ProfileSpec spec2{ .name = "pressure", .units = "Pa", .n_cells = 10 };
  std::vector<double> values2(10, 101325.0);
  warehouse.Add(Profile(spec2, values2));

  EXPECT_EQ(warehouse.Size(), 2u);

  warehouse.Clear();

  EXPECT_TRUE(warehouse.Empty());
  EXPECT_EQ(warehouse.Size(), 0u);
  EXPECT_FALSE(warehouse.Exists("temperature", "K"));
  EXPECT_FALSE(warehouse.Exists("pressure", "Pa"));
}

// ============================================================================
// ProfileWarehouse Practical Usage Tests
// ============================================================================

TEST(ProfileWarehouseTest, TuvxTypicalSetup)
{
  ProfileWarehouse warehouse;

  // Add temperature profile
  ProfileSpec temp_spec{ .name = "temperature", .units = "K", .n_cells = 80, .scale_height = 8000.0 };
  std::vector<double> temperatures(80);
  for (std::size_t i = 0; i < 80; ++i)
  {
    // Simple lapse rate model
    temperatures[i] = 288.0 - 6.5 * static_cast<double>(i);
    if (temperatures[i] < 200.0)
      temperatures[i] = 200.0;
  }
  warehouse.Add(Profile(temp_spec, temperatures));

  // Add air density profile
  ProfileSpec dens_spec{ .name = "air_density", .units = "molecules/cm^3", .n_cells = 80, .scale_height = 8000.0 };
  std::vector<double> densities(80);
  double scale_height = 8.0;  // km
  for (std::size_t i = 0; i < 80; ++i)
  {
    double alt = 0.75 + static_cast<double>(i) * 1.5;  // Midpoint altitudes
    densities[i] = 2.55e19 * std::exp(-alt / scale_height);
  }
  warehouse.Add(Profile(dens_spec, densities));

  // Add ozone profile
  ProfileSpec oz_spec{ .name = "ozone", .units = "molecules/cm^3", .n_cells = 80 };
  std::vector<double> ozone(80);
  for (std::size_t i = 0; i < 80; ++i)
  {
    double alt = 0.75 + static_cast<double>(i) * 1.5;
    // Simple ozone profile peaking around 25 km
    double peak = std::exp(-(alt - 25.0) * (alt - 25.0) / 100.0);
    ozone[i] = 5e12 * peak;
  }
  warehouse.Add(Profile(oz_spec, ozone));

  EXPECT_EQ(warehouse.Size(), 3u);

  // Access profiles for radiative transfer calculations
  const Profile& temperature = warehouse.Get("temperature", "K");
  const Profile& air_density = warehouse.Get("air_density", "molecules/cm^3");
  const Profile& ozone_profile = warehouse.Get("ozone", "molecules/cm^3");

  EXPECT_EQ(temperature.NumberOfCells(), 80u);
  EXPECT_EQ(air_density.NumberOfCells(), 80u);
  EXPECT_EQ(ozone_profile.NumberOfCells(), 80u);
}
