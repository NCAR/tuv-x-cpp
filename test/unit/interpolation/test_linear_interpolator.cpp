#include <tuvx/interpolation/linear_interpolator.hpp>

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// ============================================================================
// LinearInterpolator Basic Tests
// ============================================================================

TEST(LinearInterpolatorTest, ExactPointReproduction)
{
  LinearInterpolator interp;

  std::vector<double> source_x = { 0.0, 1.0, 2.0, 3.0 };
  std::vector<double> source_y = { 10.0, 20.0, 30.0, 40.0 };

  // Interpolate at exact source points
  std::vector<double> target_x = { 0.0, 1.0, 2.0, 3.0 };
  auto result = interp.Interpolate(target_x, source_x, source_y);

  ASSERT_EQ(result.size(), 4u);
  EXPECT_NEAR(result[0], 10.0, 1e-10);
  EXPECT_NEAR(result[1], 20.0, 1e-10);
  EXPECT_NEAR(result[2], 30.0, 1e-10);
  EXPECT_NEAR(result[3], 40.0, 1e-10);
}

TEST(LinearInterpolatorTest, MidpointInterpolation)
{
  LinearInterpolator interp;

  std::vector<double> source_x = { 0.0, 2.0, 4.0 };
  std::vector<double> source_y = { 0.0, 20.0, 40.0 };

  std::vector<double> target_x = { 1.0, 3.0 };
  auto result = interp.Interpolate(target_x, source_x, source_y);

  ASSERT_EQ(result.size(), 2u);
  EXPECT_NEAR(result[0], 10.0, 1e-10);  // Midpoint of [0, 20]
  EXPECT_NEAR(result[1], 30.0, 1e-10);  // Midpoint of [20, 40]
}

TEST(LinearInterpolatorTest, ArbitraryInterpolation)
{
  LinearInterpolator interp;

  std::vector<double> source_x = { 0.0, 10.0 };
  std::vector<double> source_y = { 100.0, 200.0 };

  std::vector<double> target_x = { 2.5, 5.0, 7.5 };
  auto result = interp.Interpolate(target_x, source_x, source_y);

  ASSERT_EQ(result.size(), 3u);
  EXPECT_NEAR(result[0], 125.0, 1e-10);  // 0.25 of way from 100 to 200
  EXPECT_NEAR(result[1], 150.0, 1e-10);  // 0.5 of way
  EXPECT_NEAR(result[2], 175.0, 1e-10);  // 0.75 of way
}

// ============================================================================
// LinearInterpolator Out of Range Tests
// ============================================================================

TEST(LinearInterpolatorTest, BelowRange)
{
  LinearInterpolator interp;

  std::vector<double> source_x = { 10.0, 20.0, 30.0 };
  std::vector<double> source_y = { 100.0, 200.0, 300.0 };

  std::vector<double> target_x = { 0.0, 5.0 };  // Below source range
  auto result = interp.Interpolate(target_x, source_x, source_y);

  ASSERT_EQ(result.size(), 2u);
  // Should return first source value (no extrapolation)
  EXPECT_NEAR(result[0], 100.0, 1e-10);
  EXPECT_NEAR(result[1], 100.0, 1e-10);
}

TEST(LinearInterpolatorTest, AboveRange)
{
  LinearInterpolator interp;

  std::vector<double> source_x = { 10.0, 20.0, 30.0 };
  std::vector<double> source_y = { 100.0, 200.0, 300.0 };

  std::vector<double> target_x = { 35.0, 50.0 };  // Above source range
  auto result = interp.Interpolate(target_x, source_x, source_y);

  ASSERT_EQ(result.size(), 2u);
  // Should return last source value (no extrapolation)
  EXPECT_NEAR(result[0], 300.0, 1e-10);
  EXPECT_NEAR(result[1], 300.0, 1e-10);
}

TEST(LinearInterpolatorTest, MixedInAndOutOfRange)
{
  LinearInterpolator interp;

  std::vector<double> source_x = { 10.0, 20.0 };
  std::vector<double> source_y = { 100.0, 200.0 };

  std::vector<double> target_x = { 5.0, 15.0, 25.0 };
  auto result = interp.Interpolate(target_x, source_x, source_y);

  ASSERT_EQ(result.size(), 3u);
  EXPECT_NEAR(result[0], 100.0, 1e-10);  // Below range
  EXPECT_NEAR(result[1], 150.0, 1e-10);  // In range
  EXPECT_NEAR(result[2], 200.0, 1e-10);  // Above range
}

// ============================================================================
// LinearInterpolator Edge Cases
// ============================================================================

TEST(LinearInterpolatorTest, SingleSourcePoint)
{
  LinearInterpolator interp;

  std::vector<double> source_x = { 5.0 };
  std::vector<double> source_y = { 50.0 };

  std::vector<double> target_x = { 0.0, 5.0, 10.0 };
  auto result = interp.Interpolate(target_x, source_x, source_y);

  ASSERT_EQ(result.size(), 3u);
  // All targets should get the single source value
  EXPECT_NEAR(result[0], 50.0, 1e-10);
  EXPECT_NEAR(result[1], 50.0, 1e-10);
  EXPECT_NEAR(result[2], 50.0, 1e-10);
}

TEST(LinearInterpolatorTest, EmptySource)
{
  LinearInterpolator interp;

  std::vector<double> source_x;
  std::vector<double> source_y;

  std::vector<double> target_x = { 1.0, 2.0 };
  auto result = interp.Interpolate(target_x, source_x, source_y);

  ASSERT_EQ(result.size(), 2u);
  EXPECT_DOUBLE_EQ(result[0], 0.0);
  EXPECT_DOUBLE_EQ(result[1], 0.0);
}

TEST(LinearInterpolatorTest, EmptyTarget)
{
  LinearInterpolator interp;

  std::vector<double> source_x = { 0.0, 1.0 };
  std::vector<double> source_y = { 10.0, 20.0 };

  std::vector<double> target_x;
  auto result = interp.Interpolate(target_x, source_x, source_y);

  EXPECT_TRUE(result.empty());
}

TEST(LinearInterpolatorTest, SizeMismatch)
{
  LinearInterpolator interp;

  std::vector<double> source_x = { 0.0, 1.0, 2.0 };
  std::vector<double> source_y = { 10.0, 20.0 };  // Mismatched size

  std::vector<double> target_x = { 0.5 };
  auto result = interp.Interpolate(target_x, source_x, source_y);

  // Should return zeros on mismatch
  ASSERT_EQ(result.size(), 1u);
  EXPECT_DOUBLE_EQ(result[0], 0.0);
}

// ============================================================================
// LinearInterpolator Non-Uniform Spacing Tests
// ============================================================================

TEST(LinearInterpolatorTest, NonUniformSourceSpacing)
{
  LinearInterpolator interp;

  // Non-uniform source grid
  std::vector<double> source_x = { 0.0, 1.0, 5.0, 10.0 };
  std::vector<double> source_y = { 0.0, 10.0, 50.0, 100.0 };

  std::vector<double> target_x = { 0.5, 3.0, 7.5 };
  auto result = interp.Interpolate(target_x, source_x, source_y);

  ASSERT_EQ(result.size(), 3u);
  EXPECT_NEAR(result[0], 5.0, 1e-10);   // Midpoint of [0,10]
  EXPECT_NEAR(result[1], 30.0, 1e-10);  // Halfway between 1 and 5 in x, so (10 + 50)/2 = 30
  EXPECT_NEAR(result[2], 75.0, 1e-10);  // Midpoint of [50,100]
}

TEST(LinearInterpolatorTest, DenseTargetGrid)
{
  LinearInterpolator interp;

  std::vector<double> source_x = { 0.0, 10.0 };
  std::vector<double> source_y = { 0.0, 100.0 };

  // Dense target grid
  std::vector<double> target_x;
  for (int i = 0; i <= 100; ++i)
  {
    target_x.push_back(static_cast<double>(i) / 10.0);  // 0.0 to 10.0
  }

  auto result = interp.Interpolate(target_x, source_x, source_y);

  ASSERT_EQ(result.size(), 101u);
  // Check a few points
  EXPECT_NEAR(result[0], 0.0, 1e-10);
  EXPECT_NEAR(result[50], 50.0, 1e-10);
  EXPECT_NEAR(result[100], 100.0, 1e-10);
}

// ============================================================================
// LinearInterpolator Properties Tests
// ============================================================================

TEST(LinearInterpolatorTest, CanExtrapolate)
{
  LinearInterpolator interp;
  EXPECT_FALSE(interp.CanExtrapolate());
}

TEST(LinearInterpolatorTest, SatisfiesInterpolatorConcept)
{
  // This is a compile-time check, but we verify the concept is satisfied
  static_assert(Interpolator<LinearInterpolator>);
  static_assert(ExtendedInterpolator<LinearInterpolator>);
  SUCCEED();
}

// ============================================================================
// LinearInterpolator Practical Application Tests
// ============================================================================

TEST(LinearInterpolatorTest, WavelengthGridInterpolation)
{
  LinearInterpolator interp;

  // Source: coarse wavelength grid with cross-sections
  std::vector<double> coarse_wavelength = { 200.0, 300.0, 400.0, 500.0 };
  std::vector<double> cross_sections = { 1e-18, 5e-19, 2e-19, 1e-19 };

  // Target: finer wavelength grid
  std::vector<double> fine_wavelength = { 200.0, 250.0, 300.0, 350.0, 400.0, 450.0, 500.0 };

  auto interpolated = interp.Interpolate(fine_wavelength, coarse_wavelength, cross_sections);

  ASSERT_EQ(interpolated.size(), 7u);
  EXPECT_NEAR(interpolated[0], 1e-18, 1e-22);   // Exact
  EXPECT_NEAR(interpolated[2], 5e-19, 1e-22);   // Exact
  EXPECT_NEAR(interpolated[4], 2e-19, 1e-22);   // Exact
  EXPECT_NEAR(interpolated[6], 1e-19, 1e-22);   // Exact

  // Check interpolated values are reasonable
  EXPECT_LT(interpolated[1], interpolated[0]);  // Decreasing trend
  EXPECT_GT(interpolated[1], interpolated[2]);
}

TEST(LinearInterpolatorTest, TemperatureProfileInterpolation)
{
  LinearInterpolator interp;

  // Source: temperature at specific altitudes
  std::vector<double> source_alt = { 0.0, 10.0, 20.0, 50.0 };
  std::vector<double> source_temp = { 288.0, 223.0, 217.0, 271.0 };

  // Target: interpolate to new altitude grid
  std::vector<double> target_alt = { 5.0, 15.0, 35.0 };

  auto interpolated = interp.Interpolate(target_alt, source_alt, source_temp);

  ASSERT_EQ(interpolated.size(), 3u);
  EXPECT_NEAR(interpolated[0], 255.5, 1e-10);   // Midpoint of 288 and 223
  EXPECT_NEAR(interpolated[1], 220.0, 1e-10);   // Midpoint of 223 and 217
  EXPECT_NEAR(interpolated[2], 244.0, 1e-10);   // 0.5 of way from 217 to 271
}
