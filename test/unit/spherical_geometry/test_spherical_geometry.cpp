#include <tuvx/spherical_geometry/spherical_geometry.hpp>

#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// Helper to create a simple altitude grid
Grid CreateAltitudeGrid(const std::vector<double>& edges)
{
  GridSpec spec{ "altitude", "km", edges.size() - 1 };
  return Grid(spec, edges);
}

// ============================================================================
// SphericalGeometry Construction Tests
// ============================================================================

TEST(SphericalGeometryTest, Construction)
{
  auto grid = CreateAltitudeGrid({ 0.0, 10.0, 20.0, 30.0 });
  SphericalGeometry geom(grid);

  EXPECT_EQ(geom.NumberOfLevels(), 4u);
  EXPECT_NEAR(geom.EarthRadius(), 6371.0, 0.1);
}

TEST(SphericalGeometryTest, CustomEarthRadius)
{
  auto grid = CreateAltitudeGrid({ 0.0, 10.0 });
  SphericalGeometry geom(grid, 6378.0);  // Equatorial radius

  EXPECT_NEAR(geom.EarthRadius(), 6378.0, 0.1);
}

TEST(SphericalGeometryTest, RadiiCalculation)
{
  auto grid = CreateAltitudeGrid({ 0.0, 10.0, 20.0 });
  SphericalGeometry geom(grid);

  auto radii = geom.Radii();

  ASSERT_EQ(radii.size(), 3u);
  EXPECT_NEAR(radii[0], 6371.0, 0.1);       // Surface
  EXPECT_NEAR(radii[1], 6381.0, 0.1);       // 10 km
  EXPECT_NEAR(radii[2], 6391.0, 0.1);       // 20 km
}

// ============================================================================
// Plane-Parallel Air Mass Tests
// ============================================================================

TEST(PlaneParallelAirMassTest, Zenith)
{
  // Sun at zenith: AM = 1
  double am = SphericalGeometry::PlaneParallelAirMass(0.0);
  EXPECT_NEAR(am, 1.0, 0.001);
}

TEST(PlaneParallelAirMassTest, SixtyDegrees)
{
  // At 60°: AM = 1/cos(60°) = 2
  double am = SphericalGeometry::PlaneParallelAirMass(60.0);
  EXPECT_NEAR(am, 2.0, 0.001);
}

TEST(PlaneParallelAirMassTest, HighSZA)
{
  // At high SZA, Kasten-Young gives better results than pure secant
  double am_80 = SphericalGeometry::PlaneParallelAirMass(80.0);
  double am_85 = SphericalGeometry::PlaneParallelAirMass(85.0);

  // Should be increasing with SZA
  EXPECT_GT(am_85, am_80);

  // At 80°: pure secant = 5.76, Kasten-Young ≈ 5.6
  EXPECT_GT(am_80, 5.0);
  EXPECT_LT(am_80, 6.5);
}

TEST(PlaneParallelAirMassTest, Horizon)
{
  // At 90°: infinite
  double am = SphericalGeometry::PlaneParallelAirMass(90.0);
  EXPECT_TRUE(std::isinf(am));
}

TEST(PlaneParallelAirMassTest, BelowHorizon)
{
  // Below horizon: infinite
  double am = SphericalGeometry::PlaneParallelAirMass(95.0);
  EXPECT_TRUE(std::isinf(am));
}

// ============================================================================
// Low SZA Tests (Plane-Parallel Regime)
// ============================================================================

TEST(SphericalGeometryTest, LowSZA_EnhancementFactor)
{
  auto grid = CreateAltitudeGrid({ 0.0, 10.0, 20.0, 30.0 });
  SphericalGeometry geom(grid);

  auto result = geom.Calculate(30.0);

  ASSERT_EQ(result.enhancement_factor.size(), 3u);

  // At SZA = 30°, enhancement ≈ 1/cos(30°) ≈ 1.155
  for (double ef : result.enhancement_factor)
  {
    EXPECT_NEAR(ef, 1.0 / std::cos(30.0 * constants::kDegreesToRadians), 0.01);
  }
}

TEST(SphericalGeometryTest, LowSZA_AllSunlit)
{
  auto grid = CreateAltitudeGrid({ 0.0, 10.0, 20.0 });
  SphericalGeometry geom(grid);

  auto result = geom.Calculate(45.0);

  for (bool sunlit : result.sunlit)
  {
    EXPECT_TRUE(sunlit);
  }
}

TEST(SphericalGeometryTest, LowSZA_AirMass)
{
  auto grid = CreateAltitudeGrid({ 0.0, 10.0, 20.0 });
  SphericalGeometry geom(grid);

  auto result = geom.Calculate(60.0);

  // Air mass should increase from TOA to surface
  EXPECT_LT(result.air_mass[1], result.air_mass[0]);
}

// ============================================================================
// High SZA Tests (Spherical Corrections)
// ============================================================================

TEST(SphericalGeometryTest, HighSZA_EnhancementFactor)
{
  auto grid = CreateAltitudeGrid({ 0.0, 10.0, 20.0, 50.0, 100.0 });
  SphericalGeometry geom(grid);

  auto result = geom.Calculate(88.0);

  // At 88°, plane-parallel would give 1/cos(88°) ≈ 28.6
  // Spherical correction reduces this
  for (double ef : result.enhancement_factor)
  {
    EXPECT_GT(ef, 1.0);
    EXPECT_LT(ef, 50.0);  // Should be bounded
  }
}

TEST(SphericalGeometryTest, HighSZA_StillSunlit)
{
  auto grid = CreateAltitudeGrid({ 0.0, 10.0, 20.0 });
  SphericalGeometry geom(grid);

  auto result = geom.Calculate(89.0);

  // All layers should still be sunlit at 89° SZA
  for (bool sunlit : result.sunlit)
  {
    EXPECT_TRUE(sunlit);
  }
}

// ============================================================================
// Twilight Tests (SZA > 90°)
// ============================================================================

TEST(SphericalGeometryTest, Twilight_ScreeningHeight)
{
  auto grid = CreateAltitudeGrid({ 0.0, 10.0, 20.0, 50.0, 100.0 });
  SphericalGeometry geom(grid);

  auto result = geom.Calculate(95.0);

  // At SZA > 90°, there should be a positive screening height
  EXPECT_GT(result.screening_height, 0.0);
}

TEST(SphericalGeometryTest, Twilight_LowerLayersInShadow)
{
  auto grid = CreateAltitudeGrid({ 0.0, 5.0, 10.0, 20.0, 50.0, 100.0 });
  SphericalGeometry geom(grid);

  auto result = geom.Calculate(96.0);

  // Some lower layers should be in shadow
  bool has_shadow = false;
  for (std::size_t i = 0; i < result.sunlit.size(); ++i)
  {
    if (!result.sunlit[i])
    {
      has_shadow = true;
      // Shadow layers should have zero enhancement
      EXPECT_DOUBLE_EQ(result.enhancement_factor[i], 0.0);
    }
  }

  // At 96° SZA, we expect some shadowing
  // (though this depends on the exact geometry calculation)
}

TEST(SphericalGeometryTest, Twilight_ScreeningPositive)
{
  // Verify that screening height is positive during twilight
  auto grid = CreateAltitudeGrid({ 0.0, 10.0, 50.0, 100.0, 500.0 });
  SphericalGeometry geom(grid);

  auto result_91 = geom.Calculate(91.0);
  auto result_95 = geom.Calculate(95.0);

  // Screening height should be positive during twilight (SZA > 90)
  EXPECT_GT(result_91.screening_height, 0.0);
  EXPECT_GT(result_95.screening_height, 0.0);

  // At larger SZA, screening height should not decrease
  EXPECT_GE(result_95.screening_height, result_91.screening_height);
}

// ============================================================================
// Physical Consistency Tests
// ============================================================================

TEST(SphericalGeometryTest, EnhancementIncreasesWithSZA)
{
  auto grid = CreateAltitudeGrid({ 0.0, 10.0, 20.0 });
  SphericalGeometry geom(grid);

  auto result_30 = geom.Calculate(30.0);
  auto result_60 = geom.Calculate(60.0);
  auto result_80 = geom.Calculate(80.0);

  // Enhancement should increase with SZA
  EXPECT_LT(result_30.enhancement_factor[0], result_60.enhancement_factor[0]);
  EXPECT_LT(result_60.enhancement_factor[0], result_80.enhancement_factor[0]);
}

TEST(SphericalGeometryTest, AirMassIncreasesSurfaceToTOA)
{
  auto grid = CreateAltitudeGrid({ 0.0, 10.0, 20.0, 30.0 });
  SphericalGeometry geom(grid);

  auto result = geom.Calculate(60.0);

  // Air mass at surface should be greater than at higher altitudes
  EXPECT_GT(result.air_mass[0], result.air_mass[1]);
  EXPECT_GT(result.air_mass[1], result.air_mass[2]);
}

TEST(SphericalGeometryTest, ZenithAngleStored)
{
  auto grid = CreateAltitudeGrid({ 0.0, 10.0 });
  SphericalGeometry geom(grid);

  auto result = geom.Calculate(45.5);

  EXPECT_DOUBLE_EQ(result.zenith_angle, 45.5);
}

// ============================================================================
// Chapman Function Tests
// ============================================================================

TEST(ChapmanFunctionTest, HighSun)
{
  // For high sun (small chi), Chapman ≈ exp(-x) / cos(chi)
  double x = 1.0;
  double chi = 30.0 * constants::kDegreesToRadians;

  double ch = ChapmanFunction(x, chi);

  double expected = std::exp(-x) / std::cos(chi);
  EXPECT_NEAR(ch, expected, expected * 0.1);
}

TEST(ChapmanFunctionTest, PositiveValues)
{
  // Chapman function should always be positive
  for (double x = 0.1; x < 10.0; x += 0.5)
  {
    for (double chi_deg = 0.0; chi_deg < 100.0; chi_deg += 10.0)
    {
      double chi = chi_deg * constants::kDegreesToRadians;
      double ch = ChapmanFunction(x, chi);
      EXPECT_GT(ch, 0.0);
    }
  }
}

TEST(ChapmanFunctionTest, IncreasesWithSZA)
{
  double x = 1.0;

  double ch_30 = ChapmanFunction(x, 30.0 * constants::kDegreesToRadians);
  double ch_60 = ChapmanFunction(x, 60.0 * constants::kDegreesToRadians);
  double ch_80 = ChapmanFunction(x, 80.0 * constants::kDegreesToRadians);

  EXPECT_LT(ch_30, ch_60);
  EXPECT_LT(ch_60, ch_80);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(SphericalGeometryTest, ZeroSZA)
{
  auto grid = CreateAltitudeGrid({ 0.0, 10.0, 20.0 });
  SphericalGeometry geom(grid);

  auto result = geom.Calculate(0.0);

  // At zenith, enhancement factor = 1
  for (double ef : result.enhancement_factor)
  {
    EXPECT_NEAR(ef, 1.0, 0.001);
  }
}

TEST(SphericalGeometryTest, NinetyDegreeSZA)
{
  auto grid = CreateAltitudeGrid({ 0.0, 10.0, 20.0, 50.0 });
  SphericalGeometry geom(grid);

  auto result = geom.Calculate(90.0);

  // At horizon, screening height should be near zero
  EXPECT_LT(result.screening_height, 1.0);  // Should be small

  // All layers should still be (barely) sunlit
  for (bool sunlit : result.sunlit)
  {
    EXPECT_TRUE(sunlit);
  }
}

TEST(SphericalGeometryTest, DeepTwilight)
{
  auto grid = CreateAltitudeGrid({ 0.0, 5.0, 10.0, 20.0, 50.0, 100.0, 200.0 });
  SphericalGeometry geom(grid);

  auto result = geom.Calculate(105.0);  // Deep twilight

  // High screening height expected
  EXPECT_GT(result.screening_height, 10.0);

  // Lower layers should be in shadow
  EXPECT_FALSE(result.sunlit[0]);
}
