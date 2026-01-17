#include <tuvx/cross_section/cross_section_warehouse.hpp>
#include <tuvx/cross_section/types/base.hpp>

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// Helper to create test cross-sections
std::unique_ptr<CrossSection> MakeTestCrossSection(const std::string& name)
{
  std::vector<double> wavelengths = { 200.0, 300.0, 400.0 };
  std::vector<double> xs = { 1e-18, 2e-18, 3e-18 };
  return std::make_unique<BaseCrossSection>(name, wavelengths, xs);
}

// ============================================================================
// CrossSectionWarehouse Basic Tests
// ============================================================================

TEST(CrossSectionWarehouseTest, DefaultConstruction)
{
  CrossSectionWarehouse warehouse;

  EXPECT_TRUE(warehouse.Empty());
  EXPECT_EQ(warehouse.Size(), 0u);
}

TEST(CrossSectionWarehouseTest, AddAndGetByName)
{
  CrossSectionWarehouse warehouse;

  auto handle = warehouse.Add(MakeTestCrossSection("O3"));

  EXPECT_TRUE(handle.IsValid());
  EXPECT_EQ(warehouse.Size(), 1u);
  EXPECT_FALSE(warehouse.Empty());

  const auto& xs = warehouse.Get("O3");
  EXPECT_EQ(xs.Name(), "O3");
}

TEST(CrossSectionWarehouseTest, AddAndGetByHandle)
{
  CrossSectionWarehouse warehouse;

  auto handle = warehouse.Add(MakeTestCrossSection("NO2"));

  const auto& xs = warehouse.Get(handle);
  EXPECT_EQ(xs.Name(), "NO2");
}

TEST(CrossSectionWarehouseTest, AddMultiple)
{
  CrossSectionWarehouse warehouse;

  auto h1 = warehouse.Add(MakeTestCrossSection("O3"));
  auto h2 = warehouse.Add(MakeTestCrossSection("NO2"));
  auto h3 = warehouse.Add(MakeTestCrossSection("HCHO"));

  EXPECT_EQ(warehouse.Size(), 3u);

  EXPECT_EQ(warehouse.Get(h1).Name(), "O3");
  EXPECT_EQ(warehouse.Get(h2).Name(), "NO2");
  EXPECT_EQ(warehouse.Get(h3).Name(), "HCHO");

  EXPECT_EQ(warehouse.Get("O3").Name(), "O3");
  EXPECT_EQ(warehouse.Get("NO2").Name(), "NO2");
  EXPECT_EQ(warehouse.Get("HCHO").Name(), "HCHO");
}

TEST(CrossSectionWarehouseTest, Exists)
{
  CrossSectionWarehouse warehouse;
  warehouse.Add(MakeTestCrossSection("O3"));

  EXPECT_TRUE(warehouse.Exists("O3"));
  EXPECT_FALSE(warehouse.Exists("NO2"));
  EXPECT_FALSE(warehouse.Exists("nonexistent"));
}

TEST(CrossSectionWarehouseTest, GetHandle)
{
  CrossSectionWarehouse warehouse;
  warehouse.Add(MakeTestCrossSection("O3"));
  warehouse.Add(MakeTestCrossSection("NO2"));

  auto handle = warehouse.GetHandle("NO2");
  EXPECT_TRUE(handle.IsValid());
  EXPECT_EQ(warehouse.Get(handle).Name(), "NO2");
}

TEST(CrossSectionWarehouseTest, Names)
{
  CrossSectionWarehouse warehouse;
  warehouse.Add(MakeTestCrossSection("O3"));
  warehouse.Add(MakeTestCrossSection("NO2"));
  warehouse.Add(MakeTestCrossSection("HCHO"));

  auto names = warehouse.Names();

  ASSERT_EQ(names.size(), 3u);
  EXPECT_EQ(names[0], "O3");
  EXPECT_EQ(names[1], "NO2");
  EXPECT_EQ(names[2], "HCHO");
}

TEST(CrossSectionWarehouseTest, Clear)
{
  CrossSectionWarehouse warehouse;
  warehouse.Add(MakeTestCrossSection("O3"));
  warehouse.Add(MakeTestCrossSection("NO2"));

  EXPECT_EQ(warehouse.Size(), 2u);

  warehouse.Clear();

  EXPECT_TRUE(warehouse.Empty());
  EXPECT_EQ(warehouse.Size(), 0u);
  EXPECT_FALSE(warehouse.Exists("O3"));
}

// ============================================================================
// CrossSectionWarehouse Error Cases
// ============================================================================

TEST(CrossSectionWarehouseTest, AddDuplicate)
{
  CrossSectionWarehouse warehouse;
  warehouse.Add(MakeTestCrossSection("O3"));

  EXPECT_THROW(warehouse.Add(MakeTestCrossSection("O3")), std::runtime_error);
}

TEST(CrossSectionWarehouseTest, AddNull)
{
  CrossSectionWarehouse warehouse;

  EXPECT_THROW(warehouse.Add(nullptr), std::runtime_error);
}

TEST(CrossSectionWarehouseTest, GetNonexistentByName)
{
  CrossSectionWarehouse warehouse;
  warehouse.Add(MakeTestCrossSection("O3"));

  EXPECT_THROW(warehouse.Get("NO2"), std::out_of_range);
}

TEST(CrossSectionWarehouseTest, GetNonexistentByHandle)
{
  CrossSectionWarehouse warehouse;
  warehouse.Add(MakeTestCrossSection("O3"));

  CrossSectionHandle invalid_handle;
  EXPECT_THROW(warehouse.Get(invalid_handle), std::out_of_range);
}

TEST(CrossSectionWarehouseTest, GetHandleNonexistent)
{
  CrossSectionWarehouse warehouse;

  EXPECT_THROW(warehouse.GetHandle("NO2"), std::out_of_range);
}

// ============================================================================
// CrossSectionHandle Tests
// ============================================================================

TEST(CrossSectionHandleTest, DefaultInvalid)
{
  CrossSectionHandle handle;
  EXPECT_FALSE(handle.IsValid());
}

TEST(CrossSectionHandleTest, ValidAfterAdd)
{
  CrossSectionWarehouse warehouse;
  auto handle = warehouse.Add(MakeTestCrossSection("O3"));

  EXPECT_TRUE(handle.IsValid());
  EXPECT_EQ(handle.Index(), 0u);
}

TEST(CrossSectionHandleTest, MultipleHandles)
{
  CrossSectionWarehouse warehouse;

  auto h1 = warehouse.Add(MakeTestCrossSection("O3"));
  auto h2 = warehouse.Add(MakeTestCrossSection("NO2"));

  EXPECT_EQ(h1.Index(), 0u);
  EXPECT_EQ(h2.Index(), 1u);
}

// ============================================================================
// CrossSectionWarehouse Move Semantics
// ============================================================================

TEST(CrossSectionWarehouseTest, MoveConstruction)
{
  CrossSectionWarehouse warehouse;
  warehouse.Add(MakeTestCrossSection("O3"));
  warehouse.Add(MakeTestCrossSection("NO2"));

  CrossSectionWarehouse moved(std::move(warehouse));

  EXPECT_EQ(moved.Size(), 2u);
  EXPECT_EQ(moved.Get("O3").Name(), "O3");
  EXPECT_EQ(moved.Get("NO2").Name(), "NO2");
}

TEST(CrossSectionWarehouseTest, MoveAssignment)
{
  CrossSectionWarehouse warehouse;
  warehouse.Add(MakeTestCrossSection("O3"));

  CrossSectionWarehouse other;
  other = std::move(warehouse);

  EXPECT_EQ(other.Size(), 1u);
  EXPECT_EQ(other.Get("O3").Name(), "O3");
}

// ============================================================================
// CrossSectionWarehouse Usage Pattern Test
// ============================================================================

TEST(CrossSectionWarehouseTest, TypicalUsagePattern)
{
  CrossSectionWarehouse warehouse;

  // Setup phase: add cross-sections and get handles for fast access
  auto o3_handle = warehouse.Add(MakeTestCrossSection("O3"));
  auto no2_handle = warehouse.Add(MakeTestCrossSection("NO2"));
  auto hcho_handle = warehouse.Add(MakeTestCrossSection("HCHO"));

  // Create a wavelength grid
  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 150.0, 250.0, 350.0, 450.0 };
  Grid wl_grid(spec, edges);

  double temperature = 298.0;

  // Computation phase: use handles for O(1) access
  auto o3_xs = warehouse.Get(o3_handle).Calculate(wl_grid, temperature);
  auto no2_xs = warehouse.Get(no2_handle).Calculate(wl_grid, temperature);
  auto hcho_xs = warehouse.Get(hcho_handle).Calculate(wl_grid, temperature);

  EXPECT_EQ(o3_xs.size(), 3u);
  EXPECT_EQ(no2_xs.size(), 3u);
  EXPECT_EQ(hcho_xs.size(), 3u);
}
