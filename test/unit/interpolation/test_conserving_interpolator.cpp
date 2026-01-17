#include <tuvx/interpolation/conserving_interpolator.hpp>

#include <cmath>
#include <numeric>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// Helper function to compute total area (integral)
double ComputeArea(const std::vector<double>& edges, const std::vector<double>& values)
{
  double area = 0.0;
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    area += values[i] * (edges[i + 1] - edges[i]);
  }
  return area;
}

// ============================================================================
// ConservingInterpolator Basic Tests
// ============================================================================

TEST(ConservingInterpolatorTest, IdenticalGrids)
{
  ConservingInterpolator interp;

  std::vector<double> edges = { 0.0, 1.0, 2.0, 3.0 };
  std::vector<double> values = { 10.0, 20.0, 30.0 };

  auto result = interp.Interpolate(edges, edges, values);

  ASSERT_EQ(result.size(), 3u);
  EXPECT_NEAR(result[0], 10.0, 1e-10);
  EXPECT_NEAR(result[1], 20.0, 1e-10);
  EXPECT_NEAR(result[2], 30.0, 1e-10);
}

TEST(ConservingInterpolatorTest, AreaConservation)
{
  ConservingInterpolator interp;

  // Source: 3 bins of width 1
  std::vector<double> source_edges = { 0.0, 1.0, 2.0, 3.0 };
  std::vector<double> source_values = { 10.0, 20.0, 30.0 };

  // Target: 6 bins of width 0.5 (finer grid)
  std::vector<double> target_edges = { 0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0 };

  auto result = interp.Interpolate(target_edges, source_edges, source_values);

  // Total area should be conserved
  double source_area = ComputeArea(source_edges, source_values);
  double target_area = ComputeArea(target_edges, result);

  EXPECT_NEAR(source_area, target_area, 1e-10);

  // Source area = 10*1 + 20*1 + 30*1 = 60
  EXPECT_NEAR(source_area, 60.0, 1e-10);
}

TEST(ConservingInterpolatorTest, CoarserTargetGrid)
{
  ConservingInterpolator interp;

  // Source: 4 bins of width 1
  std::vector<double> source_edges = { 0.0, 1.0, 2.0, 3.0, 4.0 };
  std::vector<double> source_values = { 10.0, 20.0, 30.0, 40.0 };

  // Target: 2 bins of width 2 (coarser grid)
  std::vector<double> target_edges = { 0.0, 2.0, 4.0 };

  auto result = interp.Interpolate(target_edges, source_edges, source_values);

  ASSERT_EQ(result.size(), 2u);

  // First target bin [0,2) overlaps source bins [0,1) and [1,2)
  // Area = 10*1 + 20*1 = 30, width = 2, value = 30/2 = 15
  EXPECT_NEAR(result[0], 15.0, 1e-10);

  // Second target bin [2,4) overlaps source bins [2,3) and [3,4)
  // Area = 30*1 + 40*1 = 70, width = 2, value = 70/2 = 35
  EXPECT_NEAR(result[1], 35.0, 1e-10);

  // Verify total area conservation
  double source_area = ComputeArea(source_edges, source_values);
  double target_area = ComputeArea(target_edges, result);
  EXPECT_NEAR(source_area, target_area, 1e-10);
}

TEST(ConservingInterpolatorTest, PartialOverlap)
{
  ConservingInterpolator interp;

  // Source: bin [0, 10) with value 100
  std::vector<double> source_edges = { 0.0, 10.0 };
  std::vector<double> source_values = { 100.0 };

  // Target: bin [3, 7) - partial overlap
  std::vector<double> target_edges = { 3.0, 7.0 };

  auto result = interp.Interpolate(target_edges, source_edges, source_values);

  ASSERT_EQ(result.size(), 1u);
  // Overlap is [3,7) = width 4, source contributes 100 * 4 = 400, result = 400/4 = 100
  EXPECT_NEAR(result[0], 100.0, 1e-10);
}

// ============================================================================
// ConservingInterpolator Out of Range Tests
// ============================================================================

TEST(ConservingInterpolatorTest, TargetBelowSource)
{
  ConservingInterpolator interp;

  std::vector<double> source_edges = { 10.0, 20.0, 30.0 };
  std::vector<double> source_values = { 100.0, 200.0 };

  // Target completely below source
  std::vector<double> target_edges = { 0.0, 5.0 };

  auto result = interp.Interpolate(target_edges, source_edges, source_values);

  ASSERT_EQ(result.size(), 1u);
  EXPECT_DOUBLE_EQ(result[0], 0.0);  // No overlap
}

TEST(ConservingInterpolatorTest, TargetAboveSource)
{
  ConservingInterpolator interp;

  std::vector<double> source_edges = { 0.0, 10.0, 20.0 };
  std::vector<double> source_values = { 100.0, 200.0 };

  // Target completely above source
  std::vector<double> target_edges = { 30.0, 40.0 };

  auto result = interp.Interpolate(target_edges, source_edges, source_values);

  ASSERT_EQ(result.size(), 1u);
  EXPECT_DOUBLE_EQ(result[0], 0.0);  // No overlap
}

TEST(ConservingInterpolatorTest, PartialSourceCoverage)
{
  ConservingInterpolator interp;

  // Source covers [0, 30)
  std::vector<double> source_edges = { 0.0, 10.0, 20.0, 30.0 };
  std::vector<double> source_values = { 10.0, 20.0, 30.0 };

  // Target extends beyond source on both ends
  std::vector<double> target_edges = { -10.0, 0.0, 10.0, 20.0, 30.0, 40.0 };

  auto result = interp.Interpolate(target_edges, source_edges, source_values);

  ASSERT_EQ(result.size(), 5u);
  EXPECT_DOUBLE_EQ(result[0], 0.0);     // [-10, 0): no overlap
  EXPECT_NEAR(result[1], 10.0, 1e-10);  // [0, 10): exactly source bin 0
  EXPECT_NEAR(result[2], 20.0, 1e-10);  // [10, 20): exactly source bin 1
  EXPECT_NEAR(result[3], 30.0, 1e-10);  // [20, 30): exactly source bin 2
  EXPECT_DOUBLE_EQ(result[4], 0.0);     // [30, 40): no overlap
}

// ============================================================================
// ConservingInterpolator Edge Cases
// ============================================================================

TEST(ConservingInterpolatorTest, SingleSourceBin)
{
  ConservingInterpolator interp;

  std::vector<double> source_edges = { 0.0, 10.0 };
  std::vector<double> source_values = { 50.0 };

  std::vector<double> target_edges = { 0.0, 5.0, 10.0 };

  auto result = interp.Interpolate(target_edges, source_edges, source_values);

  ASSERT_EQ(result.size(), 2u);
  // Both target bins should have same value as source (uniform)
  EXPECT_NEAR(result[0], 50.0, 1e-10);
  EXPECT_NEAR(result[1], 50.0, 1e-10);
}

TEST(ConservingInterpolatorTest, EmptySource)
{
  ConservingInterpolator interp;

  std::vector<double> source_edges;
  std::vector<double> source_values;

  std::vector<double> target_edges = { 0.0, 1.0 };

  auto result = interp.Interpolate(target_edges, source_edges, source_values);

  ASSERT_EQ(result.size(), 1u);
  EXPECT_DOUBLE_EQ(result[0], 0.0);
}

TEST(ConservingInterpolatorTest, EmptyTarget)
{
  ConservingInterpolator interp;

  std::vector<double> source_edges = { 0.0, 1.0 };
  std::vector<double> source_values = { 10.0 };

  std::vector<double> target_edges;

  auto result = interp.Interpolate(target_edges, source_edges, source_values);

  EXPECT_TRUE(result.empty());
}

TEST(ConservingInterpolatorTest, SingleTargetEdge)
{
  ConservingInterpolator interp;

  std::vector<double> source_edges = { 0.0, 1.0 };
  std::vector<double> source_values = { 10.0 };

  std::vector<double> target_edges = { 0.5 };  // Single edge = no bins

  auto result = interp.Interpolate(target_edges, source_edges, source_values);

  EXPECT_TRUE(result.empty());
}

TEST(ConservingInterpolatorTest, MismatchedSourceSizes)
{
  ConservingInterpolator interp;

  std::vector<double> source_edges = { 0.0, 1.0, 2.0 };  // 2 bins
  std::vector<double> source_values = { 10.0 };          // Only 1 value

  std::vector<double> target_edges = { 0.0, 1.0, 2.0 };

  auto result = interp.Interpolate(target_edges, source_edges, source_values);

  // Should return zeros on mismatch
  ASSERT_EQ(result.size(), 2u);
  EXPECT_DOUBLE_EQ(result[0], 0.0);
  EXPECT_DOUBLE_EQ(result[1], 0.0);
}

// ============================================================================
// ConservingInterpolator Properties Tests
// ============================================================================

TEST(ConservingInterpolatorTest, CanExtrapolate)
{
  ConservingInterpolator interp;
  EXPECT_FALSE(interp.CanExtrapolate());
}

TEST(ConservingInterpolatorTest, SatisfiesInterpolatorConcept)
{
  static_assert(Interpolator<ConservingInterpolator>);
  static_assert(ExtendedInterpolator<ConservingInterpolator>);
  SUCCEED();
}

// ============================================================================
// ConservingInterpolator Practical Application Tests
// ============================================================================

TEST(ConservingInterpolatorTest, WavelengthBinInterpolation)
{
  ConservingInterpolator interp;

  // Source: spectral irradiance on coarse wavelength bins (W/m^2/nm)
  std::vector<double> source_wl = { 200.0, 300.0, 400.0, 500.0 };
  std::vector<double> source_flux = { 0.5, 1.0, 0.8 };  // Flux density per bin

  // Target: finer wavelength grid
  std::vector<double> target_wl = { 200.0, 250.0, 300.0, 350.0, 400.0, 450.0, 500.0 };

  auto result = interp.Interpolate(target_wl, source_wl, source_flux);

  ASSERT_EQ(result.size(), 6u);

  // Verify conservation
  double source_total = ComputeArea(source_wl, source_flux);
  double target_total = ComputeArea(target_wl, result);
  EXPECT_NEAR(source_total, target_total, 1e-10);

  // Check individual bins make sense
  // [200,250) and [250,300) should both have value 0.5 (from source [200,300))
  EXPECT_NEAR(result[0], 0.5, 1e-10);
  EXPECT_NEAR(result[1], 0.5, 1e-10);
}

TEST(ConservingInterpolatorTest, AltitudeLayerInterpolation)
{
  ConservingInterpolator interp;

  // Source: density on altitude layers (molecules/cm^3)
  std::vector<double> source_alt = { 0.0, 10.0, 20.0, 30.0 };  // km
  std::vector<double> source_dens = { 2.5e19, 8e18, 2e18 };

  // Target: different altitude grid
  std::vector<double> target_alt = { 0.0, 5.0, 15.0, 25.0, 30.0 };

  auto result = interp.Interpolate(target_alt, source_alt, source_dens);

  ASSERT_EQ(result.size(), 4u);

  // Verify conservation
  double source_total = ComputeArea(source_alt, source_dens);
  double target_total = ComputeArea(target_alt, result);
  EXPECT_NEAR(source_total, target_total, 1e-10);

  // First target bin [0,5) gets half of source [0,10) value
  EXPECT_NEAR(result[0], 2.5e19, 1e-10);
}

TEST(ConservingInterpolatorTest, NonUniformGrids)
{
  ConservingInterpolator interp;

  // Non-uniform source grid
  std::vector<double> source_edges = { 0.0, 1.0, 5.0, 10.0 };
  std::vector<double> source_values = { 100.0, 50.0, 20.0 };

  // Non-uniform target grid
  std::vector<double> target_edges = { 0.0, 2.0, 8.0, 10.0 };

  auto result = interp.Interpolate(target_edges, source_edges, source_values);

  ASSERT_EQ(result.size(), 3u);

  // Verify total area conservation
  double source_area = ComputeArea(source_edges, source_values);
  double target_area = ComputeArea(target_edges, result);
  EXPECT_NEAR(source_area, target_area, 1e-10);

  // Source area = 100*1 + 50*4 + 20*5 = 100 + 200 + 100 = 400
  EXPECT_NEAR(source_area, 400.0, 1e-10);
}
