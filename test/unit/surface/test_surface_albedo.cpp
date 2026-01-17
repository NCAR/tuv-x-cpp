#include <tuvx/surface/surface_albedo.hpp>

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// ============================================================================
// Constant Albedo Tests
// ============================================================================

TEST(SurfaceAlbedoTest, ConstantConstruction)
{
  SurfaceAlbedo albedo(0.3);

  EXPECT_TRUE(albedo.IsConstant());
  EXPECT_DOUBLE_EQ(albedo.ConstantValue(), 0.3);
}

TEST(SurfaceAlbedoTest, ConstantAtWavelength)
{
  SurfaceAlbedo albedo(0.5);

  EXPECT_DOUBLE_EQ(albedo.At(300.0), 0.5);
  EXPECT_DOUBLE_EQ(albedo.At(500.0), 0.5);
  EXPECT_DOUBLE_EQ(albedo.At(800.0), 0.5);
}

TEST(SurfaceAlbedoTest, ConstantOnGrid)
{
  SurfaceAlbedo albedo(0.2);

  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 300.0, 400.0, 500.0, 600.0 };
  Grid grid(spec, edges);

  auto result = albedo.Calculate(grid);

  ASSERT_EQ(result.size(), 3u);
  EXPECT_DOUBLE_EQ(result[0], 0.2);
  EXPECT_DOUBLE_EQ(result[1], 0.2);
  EXPECT_DOUBLE_EQ(result[2], 0.2);
}

TEST(SurfaceAlbedoTest, ConstantInvalidZero)
{
  // Zero is valid (perfect absorber)
  EXPECT_NO_THROW(SurfaceAlbedo(0.0));
}

TEST(SurfaceAlbedoTest, ConstantInvalidOne)
{
  // One is valid (perfect reflector)
  EXPECT_NO_THROW(SurfaceAlbedo(1.0));
}

TEST(SurfaceAlbedoTest, ConstantInvalidNegative)
{
  EXPECT_THROW(SurfaceAlbedo(-0.1), std::invalid_argument);
}

TEST(SurfaceAlbedoTest, ConstantInvalidGreaterThanOne)
{
  EXPECT_THROW(SurfaceAlbedo(1.1), std::invalid_argument);
}

// ============================================================================
// Spectral Albedo Tests
// ============================================================================

TEST(SurfaceAlbedoTest, SpectralConstruction)
{
  std::vector<double> wavelengths = { 300.0, 500.0, 700.0 };
  std::vector<double> values = { 0.2, 0.4, 0.3 };

  SurfaceAlbedo albedo(wavelengths, values);

  EXPECT_FALSE(albedo.IsConstant());
  EXPECT_EQ(albedo.ReferenceWavelengths().size(), 3u);
  EXPECT_EQ(albedo.ReferenceAlbedo().size(), 3u);
}

TEST(SurfaceAlbedoTest, SpectralMismatchedSizes)
{
  std::vector<double> wavelengths = { 300.0, 500.0, 700.0 };
  std::vector<double> values = { 0.2, 0.4 };

  EXPECT_THROW(SurfaceAlbedo(wavelengths, values), std::invalid_argument);
}

TEST(SurfaceAlbedoTest, SpectralTooFewPoints)
{
  std::vector<double> wavelengths = { 500.0 };
  std::vector<double> values = { 0.3 };

  EXPECT_THROW(SurfaceAlbedo(wavelengths, values), std::invalid_argument);
}

TEST(SurfaceAlbedoTest, SpectralInvalidValue)
{
  std::vector<double> wavelengths = { 300.0, 500.0 };
  std::vector<double> values = { 0.3, 1.5 };  // 1.5 is invalid

  EXPECT_THROW(SurfaceAlbedo(wavelengths, values), std::invalid_argument);
}

TEST(SurfaceAlbedoTest, SpectralAtExactWavelength)
{
  std::vector<double> wavelengths = { 300.0, 500.0, 700.0 };
  std::vector<double> values = { 0.2, 0.4, 0.3 };

  SurfaceAlbedo albedo(wavelengths, values);

  EXPECT_DOUBLE_EQ(albedo.At(300.0), 0.2);
  EXPECT_DOUBLE_EQ(albedo.At(500.0), 0.4);
  EXPECT_DOUBLE_EQ(albedo.At(700.0), 0.3);
}

TEST(SurfaceAlbedoTest, SpectralAtInterpolatedWavelength)
{
  std::vector<double> wavelengths = { 300.0, 500.0 };
  std::vector<double> values = { 0.2, 0.4 };

  SurfaceAlbedo albedo(wavelengths, values);

  // Midpoint should interpolate
  EXPECT_NEAR(albedo.At(400.0), 0.3, 1e-10);
}

TEST(SurfaceAlbedoTest, SpectralAtExtrapolatedWavelength)
{
  std::vector<double> wavelengths = { 300.0, 500.0 };
  std::vector<double> values = { 0.2, 0.4 };

  SurfaceAlbedo albedo(wavelengths, values);

  // Below range - use first value
  EXPECT_DOUBLE_EQ(albedo.At(200.0), 0.2);

  // Above range - use last value
  EXPECT_DOUBLE_EQ(albedo.At(800.0), 0.4);
}

TEST(SurfaceAlbedoTest, SpectralOnGrid)
{
  std::vector<double> wavelengths = { 300.0, 500.0, 700.0 };
  std::vector<double> values = { 0.2, 0.4, 0.3 };

  SurfaceAlbedo albedo(wavelengths, values);

  GridSpec spec{ "wavelength", "nm", 2 };
  std::vector<double> edges = { 300.0, 500.0, 700.0 };  // Midpoints at 400, 600
  Grid grid(spec, edges);

  auto result = albedo.Calculate(grid);

  ASSERT_EQ(result.size(), 2u);
  // At 400 nm: interpolate 300->500 = 0.2 + 0.5*(0.4-0.2) = 0.3
  EXPECT_NEAR(result[0], 0.3, 1e-10);
  // At 600 nm: interpolate 500->700 = 0.4 + 0.5*(0.3-0.4) = 0.35
  EXPECT_NEAR(result[1], 0.35, 1e-10);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(SurfaceAlbedoTest, ConstantValueOnSpectral)
{
  std::vector<double> wavelengths = { 300.0, 500.0 };
  std::vector<double> values = { 0.2, 0.4 };

  SurfaceAlbedo albedo(wavelengths, values);

  EXPECT_THROW(albedo.ConstantValue(), std::runtime_error);
}

TEST(SurfaceAlbedoTest, ReferenceWavelengthsOnConstant)
{
  SurfaceAlbedo albedo(0.3);

  EXPECT_THROW(albedo.ReferenceWavelengths(), std::runtime_error);
}

TEST(SurfaceAlbedoTest, ReferenceAlbedoOnConstant)
{
  SurfaceAlbedo albedo(0.3);

  EXPECT_THROW(albedo.ReferenceAlbedo(), std::runtime_error);
}

// ============================================================================
// Surface Type Factory Tests
// ============================================================================

TEST(SurfaceTypesTest, Ocean)
{
  auto ocean = surface_types::Ocean();

  EXPECT_TRUE(ocean.IsConstant());
  EXPECT_GT(ocean.ConstantValue(), 0.0);
  EXPECT_LT(ocean.ConstantValue(), 0.15);
}

TEST(SurfaceTypesTest, OceanWindDependence)
{
  auto calm = surface_types::Ocean(0.0);
  auto windy = surface_types::Ocean(15.0);

  // Windy should be more reflective
  EXPECT_GT(windy.ConstantValue(), calm.ConstantValue());
}

TEST(SurfaceTypesTest, FreshSnow)
{
  auto snow = surface_types::FreshSnow();

  EXPECT_FALSE(snow.IsConstant());

  // High albedo in UV/visible
  EXPECT_GT(snow.At(400.0), 0.8);

  // Lower in near-IR
  EXPECT_LT(snow.At(1000.0), snow.At(400.0));
}

TEST(SurfaceTypesTest, Desert)
{
  auto desert = surface_types::Desert();

  EXPECT_FALSE(desert.IsConstant());

  // Lower in UV
  EXPECT_LT(desert.At(300.0), desert.At(600.0));

  // Moderate in visible
  EXPECT_GT(desert.At(600.0), 0.2);
}

TEST(SurfaceTypesTest, Vegetation)
{
  auto veg = surface_types::Vegetation();

  EXPECT_FALSE(veg.IsConstant());

  // Low in red (chlorophyll absorption) - at or below 0.2
  EXPECT_LE(veg.At(650.0), 0.2);

  // High in near-IR (red edge)
  EXPECT_GT(veg.At(800.0), 0.4);
}

TEST(SurfaceTypesTest, Urban)
{
  auto urban = surface_types::Urban();

  EXPECT_TRUE(urban.IsConstant());
  EXPECT_NEAR(urban.ConstantValue(), 0.15, 0.05);
}

TEST(SurfaceTypesTest, Forest)
{
  auto forest = surface_types::Forest();

  EXPECT_FALSE(forest.IsConstant());

  // Very dark in visible
  EXPECT_LT(forest.At(500.0), 0.1);

  // Higher in near-IR
  EXPECT_GT(forest.At(800.0), 0.3);
}

// ============================================================================
// Physical Consistency Tests
// ============================================================================

TEST(SurfaceAlbedoTest, ClampingOnGrid)
{
  // Create albedo with values that might extrapolate outside [0,1]
  // (though our implementation should prevent this)
  std::vector<double> wavelengths = { 300.0, 400.0 };
  std::vector<double> values = { 0.1, 0.9 };

  SurfaceAlbedo albedo(wavelengths, values);

  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 200.0, 300.0, 400.0, 500.0 };
  Grid grid(spec, edges);

  auto result = albedo.Calculate(grid);

  for (double a : result)
  {
    EXPECT_GE(a, 0.0);
    EXPECT_LE(a, 1.0);
  }
}

TEST(SurfaceAlbedoTest, AllTypesHaveValidRange)
{
  std::vector<SurfaceAlbedo> types = { surface_types::Ocean(),    surface_types::FreshSnow(),
                                       surface_types::Desert(),   surface_types::Vegetation(),
                                       surface_types::Urban(),    surface_types::Forest() };

  GridSpec spec{ "wavelength", "nm", 5 };
  std::vector<double> edges = { 300.0, 400.0, 500.0, 600.0, 700.0, 800.0 };
  Grid grid(spec, edges);

  for (const auto& albedo : types)
  {
    auto result = albedo.Calculate(grid);
    for (double a : result)
    {
      EXPECT_GE(a, 0.0);
      EXPECT_LE(a, 1.0);
    }
  }
}
