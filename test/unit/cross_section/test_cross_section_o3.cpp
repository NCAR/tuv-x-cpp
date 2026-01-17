#include <tuvx/cross_section/types/o3.hpp>
#include <tuvx/grid/grid.hpp>

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// ============================================================================
// O3CrossSection Construction Tests
// ============================================================================

TEST(O3CrossSectionTest, DefaultConstruction)
{
  O3CrossSection xs;

  EXPECT_EQ(xs.Name(), "O3");
  EXPECT_EQ(xs.Units(), "cm^2");

  // Check reference data was initialized
  EXPECT_GT(xs.ReferenceWavelengths().size(), 10u);
  EXPECT_GT(xs.ReferenceTemperatures().size(), 2u);
}

TEST(O3CrossSectionTest, CustomConstruction)
{
  std::vector<double> wavelengths = { 200.0, 250.0, 300.0 };
  std::vector<double> temperatures = { 220.0, 298.0 };
  std::vector<std::vector<double>> xs_data = {
    { 1e-17, 5e-19, 1e-19 },
    { 1e-17, 5e-19, 2e-19 }
  };

  O3CrossSection xs(wavelengths, temperatures, xs_data);

  EXPECT_EQ(xs.Name(), "O3");
  EXPECT_EQ(xs.ReferenceWavelengths().size(), 3u);
  EXPECT_EQ(xs.ReferenceTemperatures().size(), 2u);
}

TEST(O3CrossSectionTest, Clone)
{
  O3CrossSection original;
  auto clone = original.Clone();

  EXPECT_EQ(clone->Name(), "O3");

  O3CrossSection* o3_clone = dynamic_cast<O3CrossSection*>(clone.get());
  ASSERT_NE(o3_clone, nullptr);
  EXPECT_EQ(o3_clone->ReferenceWavelengths().size(), original.ReferenceWavelengths().size());
}

// ============================================================================
// O3CrossSection Calculation Tests
// ============================================================================

TEST(O3CrossSectionTest, CalculateAtRoomTemperature)
{
  O3CrossSection xs;

  // Create wavelength grid covering UV
  GridSpec spec{ "wavelength", "nm", 10 };
  std::vector<double> edges = { 195.0, 205.0, 215.0, 225.0, 235.0, 245.0, 255.0, 265.0, 275.0, 285.0, 295.0 };
  Grid wl_grid(spec, edges);

  auto result = xs.Calculate(wl_grid, 295.0);

  ASSERT_EQ(result.size(), 10u);

  // Cross-sections should be positive in UV
  for (std::size_t i = 0; i < result.size(); ++i)
  {
    EXPECT_GE(result[i], 0.0) << "Negative cross-section at index " << i;
  }

  // Peak absorption should be around 250-260 nm
  // Cross-sections should decrease towards longer wavelengths
  EXPECT_GT(result[2], result[9]);  // 210 nm > 290 nm
}

TEST(O3CrossSectionTest, CalculateAtLowTemperature)
{
  O3CrossSection xs;

  GridSpec spec{ "wavelength", "nm", 5 };
  std::vector<double> edges = { 295.0, 305.0, 315.0, 325.0, 335.0, 345.0 };
  Grid wl_grid(spec, edges);

  auto result_295K = xs.Calculate(wl_grid, 295.0);
  auto result_218K = xs.Calculate(wl_grid, 218.0);

  ASSERT_EQ(result_295K.size(), 5u);
  ASSERT_EQ(result_218K.size(), 5u);

  // In Huggins bands (300-350 nm), cross-section decreases at lower T
  // Check that at least one wavelength shows temperature effect
  bool temp_effect_found = false;
  for (std::size_t i = 0; i < result_295K.size(); ++i)
  {
    if (result_295K[i] > 0 && result_218K[i] > 0)
    {
      // Allow for numerical tolerance
      if (std::abs(result_295K[i] - result_218K[i]) / result_295K[i] > 0.01)
      {
        temp_effect_found = true;
        break;
      }
    }
  }
  EXPECT_TRUE(temp_effect_found) << "Expected temperature dependence in Huggins bands";
}

TEST(O3CrossSectionTest, CalculateOutsideRange)
{
  O3CrossSection xs;

  // Grid outside O3 absorption range
  GridSpec spec{ "wavelength", "nm", 2 };
  std::vector<double> edges = { 850.0, 900.0, 950.0 };
  Grid wl_grid(spec, edges);

  auto result = xs.Calculate(wl_grid, 298.0);

  ASSERT_EQ(result.size(), 2u);
  EXPECT_DOUBLE_EQ(result[0], 0.0);
  EXPECT_DOUBLE_EQ(result[1], 0.0);
}

// ============================================================================
// O3CrossSection Physical Property Tests
// ============================================================================

TEST(O3CrossSectionTest, HartleyBandPeak)
{
  O3CrossSection xs;

  // Hartley band: 200-300 nm with peak around 255 nm
  GridSpec spec{ "wavelength", "nm", 5 };
  std::vector<double> edges = { 235.0, 245.0, 255.0, 265.0, 275.0, 285.0 };
  Grid wl_grid(spec, edges);

  auto result = xs.Calculate(wl_grid, 298.0);

  // All values should be significant in Hartley band
  for (const auto& val : result)
  {
    EXPECT_GT(val, 1e-21) << "Cross-section too small in Hartley band";
    EXPECT_LT(val, 1e-16) << "Cross-section too large in Hartley band";
  }
}

TEST(O3CrossSectionTest, CrossSectionMagnitude)
{
  O3CrossSection xs;

  // Check order of magnitude at key wavelengths
  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 198.0, 202.0, 298.0, 302.0 };  // ~200nm and ~300nm
  Grid wl_grid(spec, edges);

  auto result = xs.Calculate(wl_grid, 295.0);

  // At 200 nm: ~1e-17 cm^2
  EXPECT_GT(result[0], 1e-18);
  EXPECT_LT(result[0], 1e-16);

  // At 300 nm: ~1e-19 to 1e-20 cm^2
  EXPECT_GT(result[2], 1e-22);
  EXPECT_LT(result[2], 1e-18);
}

// ============================================================================
// O3CrossSection Profile Calculation Tests
// ============================================================================

TEST(O3CrossSectionTest, CalculateProfile)
{
  O3CrossSection xs;

  // Wavelength grid
  GridSpec wl_spec{ "wavelength", "nm", 5 };
  std::vector<double> wl_edges = { 250.0, 260.0, 270.0, 280.0, 290.0, 300.0 };
  Grid wl_grid(wl_spec, wl_edges);

  // Altitude grid (3 layers)
  GridSpec alt_spec{ "altitude", "km", 3 };
  std::vector<double> alt_edges = { 0.0, 10.0, 20.0, 30.0 };
  Grid alt_grid(alt_spec, alt_edges);

  // Temperature profile: warm at surface, cold at altitude
  ProfileSpec temp_spec{ "temperature", "K", 3 };
  std::vector<double> temp_values = { 288.0, 250.0, 220.0 };
  Profile temp_profile(temp_spec, temp_values);

  auto profile = xs.CalculateProfile(wl_grid, alt_grid, temp_profile);

  ASSERT_EQ(profile.size(), 3u);  // 3 altitude layers
  for (const auto& layer : profile)
  {
    ASSERT_EQ(layer.size(), 5u);  // 5 wavelength bins
  }

  // All values should be positive
  for (std::size_t i = 0; i < profile.size(); ++i)
  {
    for (std::size_t j = 0; j < profile[i].size(); ++j)
    {
      EXPECT_GE(profile[i][j], 0.0) << "Negative value at [" << i << "][" << j << "]";
    }
  }
}

// ============================================================================
// O3CrossSection Temperature Interpolation Tests
// ============================================================================

TEST(O3CrossSectionTest, TemperatureBelowRange)
{
  O3CrossSection xs;

  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 250.0, 270.0, 290.0, 310.0 };
  Grid wl_grid(spec, edges);

  // Temperature below lowest reference (should clamp to lowest)
  auto result_150K = xs.Calculate(wl_grid, 150.0);
  auto result_218K = xs.Calculate(wl_grid, 218.0);

  // Results should be same as at lowest reference temperature
  for (std::size_t i = 0; i < result_150K.size(); ++i)
  {
    EXPECT_NEAR(result_150K[i], result_218K[i], result_218K[i] * 1e-10);
  }
}

TEST(O3CrossSectionTest, TemperatureAboveRange)
{
  O3CrossSection xs;

  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 250.0, 270.0, 290.0, 310.0 };
  Grid wl_grid(spec, edges);

  // Temperature above highest reference (should clamp to highest)
  auto result_350K = xs.Calculate(wl_grid, 350.0);
  auto result_295K = xs.Calculate(wl_grid, 295.0);

  // Results should be same as at highest reference temperature
  for (std::size_t i = 0; i < result_350K.size(); ++i)
  {
    EXPECT_NEAR(result_350K[i], result_295K[i], result_295K[i] * 1e-10);
  }
}
