#include <tuvx/grid/mutable_grid.hpp>

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// ============================================================================
// MutableGrid Construction Tests
// ============================================================================

TEST(MutableGridTest, ConstructFromEdges)
{
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 4 };
  std::vector<double> edges = { 100.0, 200.0, 300.0, 400.0, 500.0 };

  MutableGrid grid(spec, edges);

  EXPECT_EQ(grid.Name(), "wavelength");
  EXPECT_EQ(grid.Units(), "nm");
  EXPECT_EQ(grid.NumberOfCells(), 4u);
  EXPECT_EQ(grid.Edges().size(), 5u);
}

TEST(MutableGridTest, ConstructFromGrid)
{
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 5 };
  Grid immutable_grid = Grid::EquallySpaced(spec, 100.0, 600.0);

  MutableGrid mutable_grid(immutable_grid);

  EXPECT_EQ(mutable_grid.Name(), "wavelength");
  EXPECT_EQ(mutable_grid.NumberOfCells(), 5u);
  EXPECT_DOUBLE_EQ(mutable_grid.LowerBound(), 100.0);
  EXPECT_DOUBLE_EQ(mutable_grid.UpperBound(), 600.0);
}

TEST(MutableGridTest, EdgesMismatchThrows)
{
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 4 };
  std::vector<double> edges = { 100.0, 200.0, 300.0 };  // Wrong size

  EXPECT_THROW(MutableGrid(spec, edges), TuvxInternalException);
}

TEST(MutableGridTest, EquallySpaced)
{
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 5 };
  MutableGrid grid = MutableGrid::EquallySpaced(spec, 100.0, 600.0);

  EXPECT_DOUBLE_EQ(grid.LowerBound(), 100.0);
  EXPECT_DOUBLE_EQ(grid.UpperBound(), 600.0);

  auto edges = grid.Edges();
  for (std::size_t i = 1; i < edges.size(); ++i)
  {
    EXPECT_NEAR(edges[i] - edges[i - 1], 100.0, 1e-10);
  }
}

TEST(MutableGridTest, LogarithmicallySpaced)
{
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 3 };
  MutableGrid grid = MutableGrid::LogarithmicallySpaced(spec, 10.0, 10000.0);

  EXPECT_DOUBLE_EQ(grid.LowerBound(), 10.0);
  EXPECT_DOUBLE_EQ(grid.UpperBound(), 10000.0);

  auto edges = grid.Edges();
  EXPECT_NEAR(edges[0], 10.0, 1e-10);
  EXPECT_NEAR(edges[1], 100.0, 1e-8);
  EXPECT_NEAR(edges[2], 1000.0, 1e-6);
  EXPECT_NEAR(edges[3], 10000.0, 1e-4);
}

// ============================================================================
// MutableGrid Modification Tests
// ============================================================================

TEST(MutableGridTest, SetEdges)
{
  GridSpec spec{ .name = "test", .units = "m", .n_cells = 3 };
  std::vector<double> initial = { 0.0, 1.0, 2.0, 3.0 };

  MutableGrid grid(spec, initial);

  std::vector<double> new_edges = { 0.0, 2.0, 4.0, 6.0 };
  grid.SetEdges(new_edges);

  auto edges = grid.Edges();
  EXPECT_DOUBLE_EQ(edges[0], 0.0);
  EXPECT_DOUBLE_EQ(edges[1], 2.0);
  EXPECT_DOUBLE_EQ(edges[2], 4.0);
  EXPECT_DOUBLE_EQ(edges[3], 6.0);

  // Verify midpoints and deltas updated
  auto midpoints = grid.Midpoints();
  EXPECT_NEAR(midpoints[0], 1.0, 1e-10);
  EXPECT_NEAR(midpoints[1], 3.0, 1e-10);
  EXPECT_NEAR(midpoints[2], 5.0, 1e-10);

  auto deltas = grid.Deltas();
  EXPECT_NEAR(deltas[0], 2.0, 1e-10);
  EXPECT_NEAR(deltas[1], 2.0, 1e-10);
  EXPECT_NEAR(deltas[2], 2.0, 1e-10);
}

TEST(MutableGridTest, SetEdgesWrongSize)
{
  GridSpec spec{ .name = "test", .units = "m", .n_cells = 3 };
  std::vector<double> initial = { 0.0, 1.0, 2.0, 3.0 };

  MutableGrid grid(spec, initial);

  std::vector<double> wrong_size = { 0.0, 1.0, 2.0 };
  EXPECT_THROW(grid.SetEdges(wrong_size), TuvxInternalException);
}

TEST(MutableGridTest, SetSingleEdge)
{
  GridSpec spec{ .name = "test", .units = "m", .n_cells = 3 };
  std::vector<double> initial = { 0.0, 1.0, 2.0, 3.0 };

  MutableGrid grid(spec, initial);

  grid.SetEdge(1, 1.5);
  grid.Update();

  auto edges = grid.Edges();
  EXPECT_DOUBLE_EQ(edges[1], 1.5);

  // Verify midpoints updated
  auto midpoints = grid.Midpoints();
  EXPECT_NEAR(midpoints[0], 0.75, 1e-10);
  EXPECT_NEAR(midpoints[1], 1.75, 1e-10);
}

TEST(MutableGridTest, SetSingleEdgeOutOfRange)
{
  GridSpec spec{ .name = "test", .units = "m", .n_cells = 3 };
  std::vector<double> initial = { 0.0, 1.0, 2.0, 3.0 };

  MutableGrid grid(spec, initial);

  EXPECT_THROW(grid.SetEdge(10, 5.0), TuvxInternalException);
}

TEST(MutableGridTest, MutableEdgesAccess)
{
  GridSpec spec{ .name = "test", .units = "m", .n_cells = 3 };
  std::vector<double> initial = { 0.0, 1.0, 2.0, 3.0 };

  MutableGrid grid(spec, initial);

  auto mutable_edges = grid.MutableEdges();
  mutable_edges[0] = -1.0;
  mutable_edges[3] = 5.0;
  grid.Update();

  EXPECT_DOUBLE_EQ(grid.LowerBound(), -1.0);
  EXPECT_DOUBLE_EQ(grid.UpperBound(), 5.0);
}

// ============================================================================
// MutableGrid ToGrid Conversion Tests
// ============================================================================

TEST(MutableGridTest, ToGrid)
{
  GridSpec spec{ .name = "wavelength", .units = "nm", .n_cells = 4 };
  MutableGrid mutable_grid = MutableGrid::EquallySpaced(spec, 100.0, 500.0);

  Grid immutable_grid = mutable_grid.ToGrid();

  EXPECT_EQ(immutable_grid.Name(), "wavelength");
  EXPECT_EQ(immutable_grid.NumberOfCells(), 4u);
  EXPECT_DOUBLE_EQ(immutable_grid.LowerBound(), 100.0);
  EXPECT_DOUBLE_EQ(immutable_grid.UpperBound(), 500.0);
}

TEST(MutableGridTest, ToGridAfterModification)
{
  GridSpec spec{ .name = "test", .units = "m", .n_cells = 3 };
  std::vector<double> initial = { 0.0, 1.0, 2.0, 3.0 };

  MutableGrid mutable_grid(spec, initial);
  mutable_grid.SetEdge(1, 1.5);
  mutable_grid.Update();

  Grid immutable_grid = mutable_grid.ToGrid();

  auto edges = immutable_grid.Edges();
  EXPECT_DOUBLE_EQ(edges[1], 1.5);
}

// ============================================================================
// MutableGrid FindCell Tests
// ============================================================================

TEST(MutableGridTest, FindCellBasic)
{
  GridSpec spec{ .name = "test", .units = "m", .n_cells = 4 };
  std::vector<double> edges = { 0.0, 1.0, 2.0, 3.0, 4.0 };

  MutableGrid grid(spec, edges);

  EXPECT_EQ(grid.FindCell(0.5), 0u);
  EXPECT_EQ(grid.FindCell(1.5), 1u);
  EXPECT_EQ(grid.FindCell(2.5), 2u);
  EXPECT_EQ(grid.FindCell(3.5), 3u);
}

TEST(MutableGridTest, FindCellAfterModification)
{
  GridSpec spec{ .name = "test", .units = "m", .n_cells = 3 };
  std::vector<double> initial = { 0.0, 1.0, 2.0, 3.0 };

  MutableGrid grid(spec, initial);

  // Modify edges to shift cell boundaries
  std::vector<double> new_edges = { 0.0, 0.5, 1.5, 3.0 };
  grid.SetEdges(new_edges);

  // Cell 0: [0.0, 0.5)
  // Cell 1: [0.5, 1.5)
  // Cell 2: [1.5, 3.0]
  EXPECT_EQ(grid.FindCell(0.25), 0u);
  EXPECT_EQ(grid.FindCell(1.0), 1u);
  EXPECT_EQ(grid.FindCell(2.5), 2u);
}

// ============================================================================
// MutableGrid Runtime Update Scenario Tests
// ============================================================================

TEST(MutableGridTest, AdaptiveGridRefinement)
{
  // Simulate runtime grid refinement scenario
  GridSpec spec{ .name = "altitude", .units = "km", .n_cells = 4 };
  MutableGrid grid = MutableGrid::EquallySpaced(spec, 0.0, 80.0);

  // Check initial uniform spacing (20 km)
  auto deltas = grid.Deltas();
  for (const auto& d : deltas)
  {
    EXPECT_NEAR(d, 20.0, 1e-10);
  }

  // Refine grid in lower atmosphere (finer resolution near surface)
  std::vector<double> refined = { 0.0, 5.0, 15.0, 35.0, 80.0 };
  grid.SetEdges(refined);

  deltas = grid.Deltas();
  EXPECT_NEAR(deltas[0], 5.0, 1e-10);   // 0-5 km (finest)
  EXPECT_NEAR(deltas[1], 10.0, 1e-10);  // 5-15 km
  EXPECT_NEAR(deltas[2], 20.0, 1e-10);  // 15-35 km
  EXPECT_NEAR(deltas[3], 45.0, 1e-10);  // 35-80 km (coarsest)
}

TEST(MutableGridTest, TimeVaryingCoordinates)
{
  // Simulate time-varying coordinate scenario
  GridSpec spec{ .name = "time", .units = "hours", .n_cells = 4 };
  MutableGrid grid = MutableGrid::EquallySpaced(spec, 0.0, 24.0);

  EXPECT_DOUBLE_EQ(grid.LowerBound(), 0.0);
  EXPECT_DOUBLE_EQ(grid.UpperBound(), 24.0);

  // Shift time window to next day
  auto mutable_edges = grid.MutableEdges();
  for (auto& edge : mutable_edges)
  {
    edge += 24.0;
  }
  grid.Update();

  EXPECT_DOUBLE_EQ(grid.LowerBound(), 24.0);
  EXPECT_DOUBLE_EQ(grid.UpperBound(), 48.0);
}
