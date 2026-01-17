#include <tuvx/grid/grid.hpp>
#include <tuvx/grid/grid_spec.hpp>

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// ============================================================================
// GridSpec Tests
// ============================================================================

TEST(GridSpecTest, DefaultConstruction)
{
  GridSpec spec;
  EXPECT_TRUE(spec.name.empty());
  EXPECT_TRUE(spec.units.empty());
  EXPECT_EQ(spec.n_cells, 0u);
}

TEST(GridSpecTest, DesignatedInitializers)
{
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 100 };
  EXPECT_EQ(spec.name, "wavelength");
  EXPECT_EQ(spec.units, "nm");
  EXPECT_EQ(spec.n_cells, 100u);
}

TEST(GridSpecTest, Equality)
{
  GridSpec a{ .name = "wavelength", .units = "nm", .n_cells = 100 };
  GridSpec b{ .name = "wavelength", .units = "nm", .n_cells = 100 };
  GridSpec c{ .name = "altitude", .units = "nm", .n_cells = 100 };

  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
}

TEST(GridSpecTest, Key)
{
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 100 };
  EXPECT_EQ(spec.Key(), "wavelength|nm");
}

// ============================================================================
// Grid Construction Tests
// ============================================================================

TEST(GridTest, ConstructFromEdges)
{
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 4 };
  std::vector<double> edges = { 100.0, 200.0, 300.0, 400.0, 500.0 };

  Grid grid(spec, edges);

  EXPECT_EQ(grid.Name(), "wavelength");
  EXPECT_EQ(grid.Units(), "nm");
  EXPECT_EQ(grid.NumberOfCells(), 4u);
  EXPECT_EQ(grid.Edges().size(), 5u);
  EXPECT_EQ(grid.Midpoints().size(), 4u);
  EXPECT_EQ(grid.Deltas().size(), 4u);
}

TEST(GridTest, EdgesMismatchThrows)
{
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 4 };
  std::vector<double> edges = { 100.0, 200.0, 300.0 };  // Wrong size

  EXPECT_THROW(Grid(spec, edges), TuvxInternalException);
}

TEST(GridTest, EquallySpaced)
{
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 5 };
  Grid grid = Grid::EquallySpaced(spec, 100.0, 600.0);

  EXPECT_DOUBLE_EQ(grid.LowerBound(), 100.0);
  EXPECT_DOUBLE_EQ(grid.UpperBound(), 600.0);
  EXPECT_EQ(grid.Edges().size(), 6u);

  // Check uniform spacing
  auto edges = grid.Edges();
  for (std::size_t i = 1; i < edges.size(); ++i)
  {
    EXPECT_NEAR(edges[i] - edges[i - 1], 100.0, 1e-10);
  }
}

TEST(GridTest, LogarithmicallySpaced)
{
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 3 };
  Grid grid = Grid::LogarithmicallySpaced(spec, 10.0, 10000.0);

  EXPECT_DOUBLE_EQ(grid.LowerBound(), 10.0);
  EXPECT_DOUBLE_EQ(grid.UpperBound(), 10000.0);

  // Check logarithmic spacing (factor of 10 each step)
  auto edges = grid.Edges();
  EXPECT_NEAR(edges[0], 10.0, 1e-10);
  EXPECT_NEAR(edges[1], 100.0, 1e-8);
  EXPECT_NEAR(edges[2], 1000.0, 1e-6);
  EXPECT_NEAR(edges[3], 10000.0, 1e-4);
}

TEST(GridTest, LogarithmicallySpacedInvalidBounds)
{
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 3 };

  EXPECT_THROW(Grid::LogarithmicallySpaced(spec, -10.0, 100.0), TuvxInternalException);
  EXPECT_THROW(Grid::LogarithmicallySpaced(spec, 0.0, 100.0), TuvxInternalException);
  EXPECT_THROW(Grid::LogarithmicallySpaced(spec, 10.0, 0.0), TuvxInternalException);
}

// ============================================================================
// Grid Midpoint and Delta Tests
// ============================================================================

TEST(GridTest, MidpointCalculation)
{
  GridSpec spec{ .name = "test", .units = "m", .n_cells = 3 };
  std::vector<double> edges = { 0.0, 2.0, 6.0, 12.0 };

  Grid grid(spec, edges);
  auto midpoints = grid.Midpoints();

  EXPECT_NEAR(midpoints[0], 1.0, 1e-10);
  EXPECT_NEAR(midpoints[1], 4.0, 1e-10);
  EXPECT_NEAR(midpoints[2], 9.0, 1e-10);
}

TEST(GridTest, DeltaCalculation)
{
  GridSpec spec{ .name = "test", .units = "m", .n_cells = 3 };
  std::vector<double> edges = { 0.0, 2.0, 6.0, 12.0 };

  Grid grid(spec, edges);
  auto deltas = grid.Deltas();

  EXPECT_NEAR(deltas[0], 2.0, 1e-10);
  EXPECT_NEAR(deltas[1], 4.0, 1e-10);
  EXPECT_NEAR(deltas[2], 6.0, 1e-10);
}

TEST(GridTest, NonUniformSpacing)
{
  GridSpec spec{ .name = "altitude", .units = "km", .n_cells = 4 };
  std::vector<double> edges = { 0.0, 1.0, 3.0, 7.0, 15.0 };

  Grid grid(spec, edges);
  auto deltas = grid.Deltas();

  EXPECT_NEAR(deltas[0], 1.0, 1e-10);
  EXPECT_NEAR(deltas[1], 2.0, 1e-10);
  EXPECT_NEAR(deltas[2], 4.0, 1e-10);
  EXPECT_NEAR(deltas[3], 8.0, 1e-10);
}

// ============================================================================
// Grid FindCell Tests
// ============================================================================

TEST(GridTest, FindCellBasic)
{
  GridSpec spec{ .name = "test", .units = "m", .n_cells = 4 };
  std::vector<double> edges = { 0.0, 1.0, 2.0, 3.0, 4.0 };

  Grid grid(spec, edges);

  EXPECT_EQ(grid.FindCell(0.5), 0u);
  EXPECT_EQ(grid.FindCell(1.5), 1u);
  EXPECT_EQ(grid.FindCell(2.5), 2u);
  EXPECT_EQ(grid.FindCell(3.5), 3u);
}

TEST(GridTest, FindCellAtEdges)
{
  GridSpec spec{ .name = "test", .units = "m", .n_cells = 3 };
  std::vector<double> edges = { 0.0, 1.0, 2.0, 3.0 };

  Grid grid(spec, edges);

  // At lower bound - should be first cell
  EXPECT_EQ(grid.FindCell(0.0), 0u);

  // At upper bound - should be last cell
  EXPECT_EQ(grid.FindCell(3.0), 2u);

  // At internal edges
  EXPECT_EQ(grid.FindCell(1.0), 1u);
  EXPECT_EQ(grid.FindCell(2.0), 2u);
}

TEST(GridTest, FindCellOutOfRange)
{
  GridSpec spec{ .name = "test", .units = "m", .n_cells = 3 };
  std::vector<double> edges = { 0.0, 1.0, 2.0, 3.0 };

  Grid grid(spec, edges);

  EXPECT_FALSE(grid.FindCell(-0.1).has_value());
  EXPECT_FALSE(grid.FindCell(3.1).has_value());
}

TEST(GridTest, FindCellDescendingGrid)
{
  GridSpec spec{ .name = "altitude", .units = "km", .n_cells = 3 };
  std::vector<double> edges = { 100.0, 50.0, 20.0, 0.0 };  // Top to bottom

  Grid grid(spec, edges);

  EXPECT_EQ(grid.FindCell(75.0), 0u);
  EXPECT_EQ(grid.FindCell(35.0), 1u);
  EXPECT_EQ(grid.FindCell(10.0), 2u);

  // At edges
  EXPECT_EQ(grid.FindCell(100.0), 0u);
  EXPECT_EQ(grid.FindCell(0.0), 2u);

  // Out of range
  EXPECT_FALSE(grid.FindCell(101.0).has_value());
  EXPECT_FALSE(grid.FindCell(-1.0).has_value());
}

TEST(GridTest, FindCellEmptyGrid)
{
  GridSpec spec{ .name = "test", .units = "m", .n_cells = 0 };
  std::vector<double> edges = { 0.0 };

  Grid grid(spec, edges);

  EXPECT_FALSE(grid.FindCell(0.0).has_value());
}

// ============================================================================
// Grid Spec Access Tests
// ============================================================================

TEST(GridTest, SpecAccess)
{
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 10 };
  Grid grid = Grid::EquallySpaced(spec, 200.0, 800.0);

  const GridSpec& retrieved_spec = grid.Spec();
  EXPECT_EQ(retrieved_spec.name, "wavelength");
  EXPECT_EQ(retrieved_spec.units, "nm");
  EXPECT_EQ(retrieved_spec.n_cells, 10u);
}

// ============================================================================
// Grid Wavelength Example Tests
// ============================================================================

TEST(GridTest, WavelengthGridExample)
{
  // Typical UV wavelength grid
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 140 };
  Grid grid = Grid::EquallySpaced(spec, 120.0, 750.0);

  EXPECT_EQ(grid.NumberOfCells(), 140u);
  EXPECT_DOUBLE_EQ(grid.LowerBound(), 120.0);
  EXPECT_DOUBLE_EQ(grid.UpperBound(), 750.0);

  // Check a specific cell lookup
  auto cell = grid.FindCell(300.0);
  ASSERT_TRUE(cell.has_value());
  // 300 nm should be in the correct range
  auto edges = grid.Edges();
  EXPECT_LE(edges[*cell], 300.0);
  EXPECT_GE(edges[*cell + 1], 300.0);
}

// ============================================================================
// Grid Altitude Example Tests
// ============================================================================

TEST(GridTest, AltitudeGridExample)
{
  // Typical altitude grid from surface to top of atmosphere
  GridSpec spec{ .name = "altitude", .units = "km", .n_cells = 80 };
  Grid grid = Grid::EquallySpaced(spec, 0.0, 120.0);

  EXPECT_EQ(grid.NumberOfCells(), 80u);
  EXPECT_DOUBLE_EQ(grid.LowerBound(), 0.0);
  EXPECT_DOUBLE_EQ(grid.UpperBound(), 120.0);

  // Check 1.5 km spacing
  auto deltas = grid.Deltas();
  EXPECT_NEAR(deltas[0], 1.5, 1e-10);
}
