#include <tuvx/quantum_yield/quantum_yield_warehouse.hpp>
#include <tuvx/quantum_yield/types/base.hpp>

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// Helper to create test quantum yields
std::unique_ptr<QuantumYield> MakeTestQuantumYield(const std::string& name)
{
  return std::make_unique<ConstantQuantumYield>(name, "A", "B", 1.0);
}

// ============================================================================
// QuantumYieldWarehouse Basic Tests
// ============================================================================

TEST(QuantumYieldWarehouseTest, DefaultConstruction)
{
  QuantumYieldWarehouse warehouse;

  EXPECT_TRUE(warehouse.Empty());
  EXPECT_EQ(warehouse.Size(), 0u);
}

TEST(QuantumYieldWarehouseTest, AddAndGetByName)
{
  QuantumYieldWarehouse warehouse;

  auto handle = warehouse.Add(MakeTestQuantumYield("O3->O(1D)+O2"));

  EXPECT_TRUE(handle.IsValid());
  EXPECT_EQ(warehouse.Size(), 1u);
  EXPECT_FALSE(warehouse.Empty());

  const auto& qy = warehouse.Get("O3->O(1D)+O2");
  EXPECT_EQ(qy.Name(), "O3->O(1D)+O2");
}

TEST(QuantumYieldWarehouseTest, AddAndGetByHandle)
{
  QuantumYieldWarehouse warehouse;

  auto handle = warehouse.Add(MakeTestQuantumYield("NO2->NO+O"));

  const auto& qy = warehouse.Get(handle);
  EXPECT_EQ(qy.Name(), "NO2->NO+O");
}

TEST(QuantumYieldWarehouseTest, AddMultiple)
{
  QuantumYieldWarehouse warehouse;

  auto h1 = warehouse.Add(MakeTestQuantumYield("O3->O(1D)+O2"));
  auto h2 = warehouse.Add(MakeTestQuantumYield("NO2->NO+O"));
  auto h3 = warehouse.Add(MakeTestQuantumYield("HCHO->H+HCO"));

  EXPECT_EQ(warehouse.Size(), 3u);

  EXPECT_EQ(warehouse.Get(h1).Name(), "O3->O(1D)+O2");
  EXPECT_EQ(warehouse.Get(h2).Name(), "NO2->NO+O");
  EXPECT_EQ(warehouse.Get(h3).Name(), "HCHO->H+HCO");
}

TEST(QuantumYieldWarehouseTest, Exists)
{
  QuantumYieldWarehouse warehouse;
  warehouse.Add(MakeTestQuantumYield("O3->O(1D)+O2"));

  EXPECT_TRUE(warehouse.Exists("O3->O(1D)+O2"));
  EXPECT_FALSE(warehouse.Exists("NO2->NO+O"));
  EXPECT_FALSE(warehouse.Exists("nonexistent"));
}

TEST(QuantumYieldWarehouseTest, GetHandle)
{
  QuantumYieldWarehouse warehouse;
  warehouse.Add(MakeTestQuantumYield("O3->O(1D)+O2"));
  warehouse.Add(MakeTestQuantumYield("NO2->NO+O"));

  auto handle = warehouse.GetHandle("NO2->NO+O");
  EXPECT_TRUE(handle.IsValid());
  EXPECT_EQ(warehouse.Get(handle).Name(), "NO2->NO+O");
}

TEST(QuantumYieldWarehouseTest, Names)
{
  QuantumYieldWarehouse warehouse;
  warehouse.Add(MakeTestQuantumYield("O3->O(1D)+O2"));
  warehouse.Add(MakeTestQuantumYield("NO2->NO+O"));
  warehouse.Add(MakeTestQuantumYield("HCHO->H+HCO"));

  auto names = warehouse.Names();

  ASSERT_EQ(names.size(), 3u);
  EXPECT_EQ(names[0], "O3->O(1D)+O2");
  EXPECT_EQ(names[1], "NO2->NO+O");
  EXPECT_EQ(names[2], "HCHO->H+HCO");
}

TEST(QuantumYieldWarehouseTest, Clear)
{
  QuantumYieldWarehouse warehouse;
  warehouse.Add(MakeTestQuantumYield("O3->O(1D)+O2"));
  warehouse.Add(MakeTestQuantumYield("NO2->NO+O"));

  EXPECT_EQ(warehouse.Size(), 2u);

  warehouse.Clear();

  EXPECT_TRUE(warehouse.Empty());
  EXPECT_EQ(warehouse.Size(), 0u);
  EXPECT_FALSE(warehouse.Exists("O3->O(1D)+O2"));
}

// ============================================================================
// QuantumYieldWarehouse Error Cases
// ============================================================================

TEST(QuantumYieldWarehouseTest, AddDuplicate)
{
  QuantumYieldWarehouse warehouse;
  warehouse.Add(MakeTestQuantumYield("O3->O(1D)+O2"));

  EXPECT_THROW(warehouse.Add(MakeTestQuantumYield("O3->O(1D)+O2")), std::runtime_error);
}

TEST(QuantumYieldWarehouseTest, AddNull)
{
  QuantumYieldWarehouse warehouse;

  EXPECT_THROW(warehouse.Add(nullptr), std::runtime_error);
}

TEST(QuantumYieldWarehouseTest, GetNonexistentByName)
{
  QuantumYieldWarehouse warehouse;
  warehouse.Add(MakeTestQuantumYield("O3->O(1D)+O2"));

  EXPECT_THROW(warehouse.Get("NO2->NO+O"), std::out_of_range);
}

TEST(QuantumYieldWarehouseTest, GetNonexistentByHandle)
{
  QuantumYieldWarehouse warehouse;
  warehouse.Add(MakeTestQuantumYield("O3->O(1D)+O2"));

  QuantumYieldHandle invalid_handle;
  EXPECT_THROW(warehouse.Get(invalid_handle), std::out_of_range);
}

TEST(QuantumYieldWarehouseTest, GetHandleNonexistent)
{
  QuantumYieldWarehouse warehouse;

  EXPECT_THROW(warehouse.GetHandle("NO2->NO+O"), std::out_of_range);
}

// ============================================================================
// QuantumYieldHandle Tests
// ============================================================================

TEST(QuantumYieldHandleTest, DefaultInvalid)
{
  QuantumYieldHandle handle;
  EXPECT_FALSE(handle.IsValid());
}

TEST(QuantumYieldHandleTest, ValidAfterAdd)
{
  QuantumYieldWarehouse warehouse;
  auto handle = warehouse.Add(MakeTestQuantumYield("Test"));

  EXPECT_TRUE(handle.IsValid());
  EXPECT_EQ(handle.Index(), 0u);
}

// ============================================================================
// QuantumYieldWarehouse Move Semantics
// ============================================================================

TEST(QuantumYieldWarehouseTest, MoveConstruction)
{
  QuantumYieldWarehouse warehouse;
  warehouse.Add(MakeTestQuantumYield("O3->O(1D)+O2"));
  warehouse.Add(MakeTestQuantumYield("NO2->NO+O"));

  QuantumYieldWarehouse moved(std::move(warehouse));

  EXPECT_EQ(moved.Size(), 2u);
  EXPECT_EQ(moved.Get("O3->O(1D)+O2").Name(), "O3->O(1D)+O2");
}

TEST(QuantumYieldWarehouseTest, MoveAssignment)
{
  QuantumYieldWarehouse warehouse;
  warehouse.Add(MakeTestQuantumYield("Test"));

  QuantumYieldWarehouse other;
  other = std::move(warehouse);

  EXPECT_EQ(other.Size(), 1u);
  EXPECT_EQ(other.Get("Test").Name(), "Test");
}

// ============================================================================
// QuantumYieldWarehouse Usage Pattern Test
// ============================================================================

TEST(QuantumYieldWarehouseTest, TypicalUsagePattern)
{
  QuantumYieldWarehouse warehouse;

  // Setup: add quantum yields and store handles
  auto o3_o1d_handle = warehouse.Add(MakeTestQuantumYield("O3->O(1D)+O2"));
  auto o3_o3p_handle = warehouse.Add(MakeTestQuantumYield("O3->O(3P)+O2"));
  auto no2_handle = warehouse.Add(MakeTestQuantumYield("NO2->NO+O"));

  // Create wavelength grid
  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 200.0, 300.0, 400.0, 500.0 };
  Grid wl_grid(spec, edges);

  double temperature = 298.0;
  double air_density = 2.5e19;

  // Computation: use handles for O(1) access
  auto o3_o1d_qy = warehouse.Get(o3_o1d_handle).Calculate(wl_grid, temperature, air_density);
  auto o3_o3p_qy = warehouse.Get(o3_o3p_handle).Calculate(wl_grid, temperature, air_density);
  auto no2_qy = warehouse.Get(no2_handle).Calculate(wl_grid, temperature, air_density);

  EXPECT_EQ(o3_o1d_qy.size(), 3u);
  EXPECT_EQ(o3_o3p_qy.size(), 3u);
  EXPECT_EQ(no2_qy.size(), 3u);
}
