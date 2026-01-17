#include <tuvx/grid/grid_warehouse.hpp>

#include <gtest/gtest.h>

using namespace tuvx;

// ============================================================================
// GridHandle Tests
// ============================================================================

TEST(GridHandleTest, DefaultIsInvalid)
{
  GridHandle handle;
  EXPECT_FALSE(handle.IsValid());
}

TEST(GridHandleTest, EqualityComparison)
{
  GridHandle h1;
  GridHandle h2;
  EXPECT_EQ(h1, h2);  // Both invalid
}

// ============================================================================
// GridWarehouse Basic Tests
// ============================================================================

TEST(GridWarehouseTest, InitiallyEmpty)
{
  GridWarehouse warehouse;
  EXPECT_TRUE(warehouse.Empty());
  EXPECT_EQ(warehouse.Size(), 0u);
}

TEST(GridWarehouseTest, AddAndGet)
{
  GridWarehouse warehouse;
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 10 };
  Grid grid = Grid::EquallySpaced(spec, 200.0, 800.0);

  GridHandle handle = warehouse.Add(std::move(grid));

  EXPECT_TRUE(handle.IsValid());
  EXPECT_EQ(warehouse.Size(), 1u);
  EXPECT_FALSE(warehouse.Empty());

  const Grid& retrieved = warehouse.Get(handle);
  EXPECT_EQ(retrieved.Name(), "wavelength");
  EXPECT_EQ(retrieved.Units(), "nm");
  EXPECT_EQ(retrieved.NumberOfCells(), 10u);
}

TEST(GridWarehouseTest, GetByNameAndUnits)
{
  GridWarehouse warehouse;
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 10 };
  Grid grid = Grid::EquallySpaced(spec, 200.0, 800.0);

  warehouse.Add(std::move(grid));

  const Grid& retrieved = warehouse.Get("wavelength", "nm");
  EXPECT_EQ(retrieved.Name(), "wavelength");
  EXPECT_DOUBLE_EQ(retrieved.LowerBound(), 200.0);
}

TEST(GridWarehouseTest, ExistsCheck)
{
  GridWarehouse warehouse;
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 10 };
  Grid grid = Grid::EquallySpaced(spec, 200.0, 800.0);

  warehouse.Add(std::move(grid));

  EXPECT_TRUE(warehouse.Exists("wavelength", "nm"));
  EXPECT_FALSE(warehouse.Exists("wavelength", "um"));
  EXPECT_FALSE(warehouse.Exists("altitude", "nm"));
}

TEST(GridWarehouseTest, GetHandle)
{
  GridWarehouse warehouse;
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 10 };
  Grid grid = Grid::EquallySpaced(spec, 200.0, 800.0);

  GridHandle add_handle = warehouse.Add(std::move(grid));
  GridHandle lookup_handle = warehouse.GetHandle("wavelength", "nm");

  EXPECT_TRUE(lookup_handle.IsValid());
  EXPECT_EQ(add_handle, lookup_handle);
}

TEST(GridWarehouseTest, GetHandleNotFound)
{
  GridWarehouse warehouse;
  GridHandle handle = warehouse.GetHandle("nonexistent", "units");
  EXPECT_FALSE(handle.IsValid());
}

// ============================================================================
// GridWarehouse Multiple Grids Tests
// ============================================================================

TEST(GridWarehouseTest, MultipleGrids)
{
  GridWarehouse warehouse;

  GridSpec wave_spec{ .name = "wavelength", .units = "nm", .n_cells = 100 };
  Grid wave_grid = Grid::EquallySpaced(wave_spec, 200.0, 800.0);
  GridHandle wave_handle = warehouse.Add(std::move(wave_grid));

  GridSpec alt_spec{ .name = "altitude", .units = "km", .n_cells = 80 };
  Grid alt_grid = Grid::EquallySpaced(alt_spec, 0.0, 120.0);
  GridHandle alt_handle = warehouse.Add(std::move(alt_grid));

  GridSpec time_spec{ .name = "time", .units = "hours", .n_cells = 24 };
  Grid time_grid = Grid::EquallySpaced(time_spec, 0.0, 24.0);
  GridHandle time_handle = warehouse.Add(std::move(time_grid));

  EXPECT_EQ(warehouse.Size(), 3u);

  EXPECT_EQ(warehouse.Get(wave_handle).Name(), "wavelength");
  EXPECT_EQ(warehouse.Get(alt_handle).Name(), "altitude");
  EXPECT_EQ(warehouse.Get(time_handle).Name(), "time");

  EXPECT_EQ(warehouse.Get("wavelength", "nm").NumberOfCells(), 100u);
  EXPECT_EQ(warehouse.Get("altitude", "km").NumberOfCells(), 80u);
  EXPECT_EQ(warehouse.Get("time", "hours").NumberOfCells(), 24u);
}

TEST(GridWarehouseTest, SameNameDifferentUnits)
{
  GridWarehouse warehouse;

  GridSpec nm_spec{ .name = "wavelength", .units = "nm", .n_cells = 100 };
  Grid nm_grid = Grid::EquallySpaced(nm_spec, 200.0, 800.0);
  warehouse.Add(std::move(nm_grid));

  GridSpec um_spec{ .name = "wavelength", .units = "um", .n_cells = 50 };
  Grid um_grid = Grid::EquallySpaced(um_spec, 0.2, 0.8);
  warehouse.Add(std::move(um_grid));

  EXPECT_EQ(warehouse.Size(), 2u);
  EXPECT_EQ(warehouse.Get("wavelength", "nm").NumberOfCells(), 100u);
  EXPECT_EQ(warehouse.Get("wavelength", "um").NumberOfCells(), 50u);
}

// ============================================================================
// GridWarehouse Error Handling Tests
// ============================================================================

TEST(GridWarehouseTest, DuplicateRejection)
{
  GridWarehouse warehouse;

  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 10 };
  Grid grid1 = Grid::EquallySpaced(spec, 200.0, 800.0);
  warehouse.Add(std::move(grid1));

  Grid grid2 = Grid::EquallySpaced(spec, 300.0, 700.0);
  EXPECT_THROW(warehouse.Add(std::move(grid2)), TuvxInternalException);
}

TEST(GridWarehouseTest, GetNotFoundThrows)
{
  GridWarehouse warehouse;
  EXPECT_THROW(warehouse.Get("nonexistent", "units"), TuvxInternalException);
}

TEST(GridWarehouseTest, GetInvalidHandleThrows)
{
  GridWarehouse warehouse;
  GridHandle invalid_handle;
  EXPECT_THROW(warehouse.Get(invalid_handle), TuvxInternalException);
}

// ============================================================================
// GridWarehouse Keys Tests
// ============================================================================

TEST(GridWarehouseTest, Keys)
{
  GridWarehouse warehouse;

  GridSpec wave_spec{ .name = "wavelength", .units = "nm", .n_cells = 10 };
  warehouse.Add(Grid::EquallySpaced(wave_spec, 200.0, 800.0));

  GridSpec alt_spec{ .name = "altitude", .units = "km", .n_cells = 10 };
  warehouse.Add(Grid::EquallySpaced(alt_spec, 0.0, 100.0));

  auto keys = warehouse.Keys();
  EXPECT_EQ(keys.size(), 2u);

  // Keys should be in insertion order
  EXPECT_EQ(keys[0], "wavelength|nm");
  EXPECT_EQ(keys[1], "altitude|km");
}

// ============================================================================
// GridWarehouse Clear Tests
// ============================================================================

TEST(GridWarehouseTest, Clear)
{
  GridWarehouse warehouse;

  GridSpec spec1{ .name = "wavelength", .units = "nm", .n_cells = 10 };
  warehouse.Add(Grid::EquallySpaced(spec1, 200.0, 800.0));

  GridSpec spec2{ .name = "altitude", .units = "km", .n_cells = 10 };
  warehouse.Add(Grid::EquallySpaced(spec2, 0.0, 100.0));

  EXPECT_EQ(warehouse.Size(), 2u);

  warehouse.Clear();

  EXPECT_TRUE(warehouse.Empty());
  EXPECT_EQ(warehouse.Size(), 0u);
  EXPECT_FALSE(warehouse.Exists("wavelength", "nm"));
  EXPECT_FALSE(warehouse.Exists("altitude", "km"));
}

// ============================================================================
// GridWarehouse Practical Usage Tests
// ============================================================================

TEST(GridWarehouseTest, TuvxTypicalSetup)
{
  GridWarehouse warehouse;

  // Add wavelength grid
  GridSpec wave_spec{ .name = "wavelength", .units = "nm", .n_cells = 140 };
  warehouse.Add(Grid::EquallySpaced(wave_spec, 120.0, 750.0));

  // Add altitude grid
  GridSpec alt_spec{ .name = "altitude", .units = "km", .n_cells = 80 };
  warehouse.Add(Grid::EquallySpaced(alt_spec, 0.0, 120.0));

  // Add solar zenith angle grid
  GridSpec sza_spec{ .name = "solar_zenith_angle", .units = "degrees", .n_cells = 9 };
  warehouse.Add(Grid::EquallySpaced(sza_spec, 0.0, 90.0));

  EXPECT_EQ(warehouse.Size(), 3u);

  // Access grids for radiative transfer calculations
  const Grid& wavelength = warehouse.Get("wavelength", "nm");
  const Grid& altitude = warehouse.Get("altitude", "km");

  EXPECT_EQ(wavelength.NumberOfCells(), 140u);
  EXPECT_EQ(altitude.NumberOfCells(), 80u);
  EXPECT_NEAR(altitude.Deltas()[0], 1.5, 1e-10);
}
