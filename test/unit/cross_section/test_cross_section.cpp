#include <tuvx/cross_section/cross_section.hpp>
#include <tuvx/cross_section/temperature_based.hpp>
#include <tuvx/cross_section/types/base.hpp>
#include <tuvx/grid/grid.hpp>

#include <cmath>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// ============================================================================
// BaseCrossSection Construction Tests
// ============================================================================

TEST(BaseCrossSectionTest, ConstructTemperatureIndependent)
{
  std::vector<double> wavelengths = { 200.0, 250.0, 300.0, 350.0, 400.0 };
  std::vector<double> xs_values = { 1e-18, 5e-19, 2e-19, 1e-19, 5e-20 };

  BaseCrossSection xs("TestSpecies", wavelengths, xs_values);

  EXPECT_EQ(xs.Name(), "TestSpecies");
  EXPECT_EQ(xs.Units(), "cm^2");
  EXPECT_FALSE(xs.IsTemperatureDependent());
  EXPECT_EQ(xs.ReferenceWavelengths().size(), 5u);
  EXPECT_EQ(xs.ReferenceTemperatures().size(), 1u);
  EXPECT_DOUBLE_EQ(xs.ReferenceTemperatures()[0], 298.0);
}

TEST(BaseCrossSectionTest, ConstructTemperatureDependent)
{
  std::vector<double> wavelengths = { 200.0, 300.0, 400.0 };
  std::vector<double> temperatures = { 220.0, 260.0, 298.0 };
  std::vector<std::vector<double>> xs_data = {
    { 1e-18, 2e-18, 3e-18 },  // 220 K
    { 1.5e-18, 2.5e-18, 3.5e-18 },  // 260 K
    { 2e-18, 3e-18, 4e-18 }  // 298 K
  };

  BaseCrossSection xs("TestSpecies", wavelengths, temperatures, xs_data);

  EXPECT_EQ(xs.Name(), "TestSpecies");
  EXPECT_TRUE(xs.IsTemperatureDependent());
  EXPECT_EQ(xs.ReferenceTemperatures().size(), 3u);
}

TEST(BaseCrossSectionTest, Clone)
{
  std::vector<double> wavelengths = { 200.0, 300.0, 400.0 };
  std::vector<double> xs_values = { 1e-18, 5e-19, 2e-19 };

  BaseCrossSection original("Species", wavelengths, xs_values);
  auto clone = original.Clone();

  EXPECT_EQ(clone->Name(), original.Name());
  EXPECT_EQ(clone->Units(), original.Units());

  // Verify clone is independent
  BaseCrossSection* base_clone = dynamic_cast<BaseCrossSection*>(clone.get());
  ASSERT_NE(base_clone, nullptr);
  EXPECT_EQ(base_clone->ReferenceWavelengths().size(), wavelengths.size());
}

// ============================================================================
// BaseCrossSection Calculation Tests
// ============================================================================

TEST(BaseCrossSectionTest, CalculateExactWavelengths)
{
  // Create cross-section with simple data
  std::vector<double> ref_wavelengths = { 200.0, 300.0, 400.0 };
  std::vector<double> ref_xs = { 1e-18, 2e-18, 3e-18 };

  BaseCrossSection xs("Test", ref_wavelengths, ref_xs);

  // Create wavelength grid with same midpoints
  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 175.0, 225.0, 275.0, 325.0 };  // Midpoints: 200, 250, 300
  // Actually let's use edges that give exact midpoints
  edges = { 150.0, 250.0, 350.0, 450.0 };  // Midpoints: 200, 300, 400
  Grid wl_grid(spec, edges);

  auto result = xs.Calculate(wl_grid, 298.0);

  ASSERT_EQ(result.size(), 3u);
  EXPECT_NEAR(result[0], 1e-18, 1e-22);
  EXPECT_NEAR(result[1], 2e-18, 1e-22);
  EXPECT_NEAR(result[2], 3e-18, 1e-22);
}

TEST(BaseCrossSectionTest, CalculateInterpolatedWavelengths)
{
  std::vector<double> ref_wavelengths = { 200.0, 300.0, 400.0 };
  std::vector<double> ref_xs = { 1e-18, 2e-18, 3e-18 };

  BaseCrossSection xs("Test", ref_wavelengths, ref_xs);

  // Create wavelength grid with midpoint at 250 nm
  GridSpec spec{ "wavelength", "nm", 1 };
  std::vector<double> edges = { 200.0, 300.0 };  // Midpoint: 250
  Grid wl_grid(spec, edges);

  auto result = xs.Calculate(wl_grid, 298.0);

  ASSERT_EQ(result.size(), 1u);
  // Linear interpolation: (250-200)/(300-200) = 0.5 -> 1e-18 + 0.5*(2e-18-1e-18) = 1.5e-18
  EXPECT_NEAR(result[0], 1.5e-18, 1e-22);
}

TEST(BaseCrossSectionTest, CalculateOutOfRange)
{
  std::vector<double> ref_wavelengths = { 200.0, 300.0, 400.0 };
  std::vector<double> ref_xs = { 1e-18, 2e-18, 3e-18 };

  BaseCrossSection xs("Test", ref_wavelengths, ref_xs);

  // Create wavelength grid outside reference range
  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 100.0, 150.0, 450.0, 500.0 };  // Midpoints: 125, 300, 475
  Grid wl_grid(spec, edges);

  auto result = xs.Calculate(wl_grid, 298.0);

  ASSERT_EQ(result.size(), 3u);
  EXPECT_DOUBLE_EQ(result[0], 0.0);  // 125 nm < 200 nm
  // result[1] is within range (300 nm midpoint)
  EXPECT_DOUBLE_EQ(result[2], 0.0);  // 475 nm > 400 nm
}

// ============================================================================
// Temperature Interpolation Tests
// ============================================================================

TEST(BaseCrossSectionTest, CalculateTemperatureInterpolation)
{
  std::vector<double> wavelengths = { 200.0, 300.0, 400.0 };
  std::vector<double> temperatures = { 200.0, 300.0 };
  std::vector<std::vector<double>> xs_data = {
    { 1e-18, 2e-18, 3e-18 },  // 200 K
    { 2e-18, 4e-18, 6e-18 }   // 300 K
  };

  BaseCrossSection xs("Test", wavelengths, temperatures, xs_data);

  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 150.0, 250.0, 350.0, 450.0 };
  Grid wl_grid(spec, edges);

  // At 250 K (midpoint between 200 and 300 K)
  auto result = xs.Calculate(wl_grid, 250.0);

  ASSERT_EQ(result.size(), 3u);
  // Linear interp: 1e-18 + 0.5*(2e-18-1e-18) = 1.5e-18
  EXPECT_NEAR(result[0], 1.5e-18, 1e-22);
  EXPECT_NEAR(result[1], 3e-18, 1e-22);
  EXPECT_NEAR(result[2], 4.5e-18, 1e-22);
}

TEST(BaseCrossSectionTest, CalculateTemperatureAtBoundary)
{
  std::vector<double> wavelengths = { 200.0, 300.0 };
  std::vector<double> temperatures = { 200.0, 300.0 };
  std::vector<std::vector<double>> xs_data = {
    { 1e-18, 2e-18 },
    { 2e-18, 4e-18 }
  };

  BaseCrossSection xs("Test", wavelengths, temperatures, xs_data);

  GridSpec spec{ "wavelength", "nm", 2 };
  std::vector<double> edges = { 150.0, 250.0, 350.0 };
  Grid wl_grid(spec, edges);

  // At lower boundary
  auto result_lo = xs.Calculate(wl_grid, 200.0);
  EXPECT_NEAR(result_lo[0], 1e-18, 1e-22);

  // At upper boundary
  auto result_hi = xs.Calculate(wl_grid, 300.0);
  EXPECT_NEAR(result_hi[0], 2e-18, 1e-22);

  // Below lower boundary (should clamp to 200 K values)
  auto result_below = xs.Calculate(wl_grid, 150.0);
  EXPECT_NEAR(result_below[0], 1e-18, 1e-22);

  // Above upper boundary (should clamp to 300 K values)
  auto result_above = xs.Calculate(wl_grid, 350.0);
  EXPECT_NEAR(result_above[0], 2e-18, 1e-22);
}

// ============================================================================
// TemperatureBasedCrossSection Utility Tests
// ============================================================================

TEST(TemperatureBasedTest, InterpolateTemperature)
{
  std::vector<double> temps = { 200.0, 250.0, 300.0 };
  std::vector<std::vector<double>> data = {
    { 1.0, 2.0 },  // 200 K
    { 2.0, 4.0 },  // 250 K
    { 3.0, 6.0 }   // 300 K
  };

  using TempInterp = TemperatureBasedCrossSection<void>;

  // Exact match
  auto result = TempInterp::InterpolateTemperature(250.0, temps, data);
  ASSERT_EQ(result.size(), 2u);
  EXPECT_NEAR(result[0], 2.0, 1e-10);
  EXPECT_NEAR(result[1], 4.0, 1e-10);

  // Interpolated
  result = TempInterp::InterpolateTemperature(225.0, temps, data);
  EXPECT_NEAR(result[0], 1.5, 1e-10);  // Midpoint between 1.0 and 2.0
  EXPECT_NEAR(result[1], 3.0, 1e-10);  // Midpoint between 2.0 and 4.0
}

TEST(TemperatureBasedTest, InterpolateTemperatureClampBounds)
{
  std::vector<double> temps = { 200.0, 300.0 };
  std::vector<std::vector<double>> data = {
    { 1.0, 2.0 },
    { 3.0, 4.0 }
  };

  using TempInterp = TemperatureBasedCrossSection<void>;

  // Below range - should return data at 200 K
  auto result = TempInterp::InterpolateTemperature(100.0, temps, data);
  EXPECT_NEAR(result[0], 1.0, 1e-10);
  EXPECT_NEAR(result[1], 2.0, 1e-10);

  // Above range - should return data at 300 K
  result = TempInterp::InterpolateTemperature(400.0, temps, data);
  EXPECT_NEAR(result[0], 3.0, 1e-10);
  EXPECT_NEAR(result[1], 4.0, 1e-10);
}

TEST(TemperatureBasedTest, FindTemperatureBounds)
{
  std::vector<double> temps = { 200.0, 250.0, 300.0 };

  using TempInterp = TemperatureBasedCrossSection<void>;

  std::size_t lo, hi;
  double weight;

  // Within range
  bool in_range = TempInterp::FindTemperatureBounds(225.0, temps, lo, hi, weight);
  EXPECT_TRUE(in_range);
  EXPECT_EQ(lo, 0u);
  EXPECT_EQ(hi, 1u);
  EXPECT_NEAR(weight, 0.5, 1e-10);

  // Below range
  in_range = TempInterp::FindTemperatureBounds(150.0, temps, lo, hi, weight);
  EXPECT_FALSE(in_range);
  EXPECT_EQ(lo, 0u);
  EXPECT_EQ(hi, 0u);

  // Above range
  in_range = TempInterp::FindTemperatureBounds(350.0, temps, lo, hi, weight);
  EXPECT_FALSE(in_range);
  EXPECT_EQ(lo, 2u);
  EXPECT_EQ(hi, 2u);
}

// ============================================================================
// Profile Calculation Tests
// ============================================================================

TEST(BaseCrossSectionTest, CalculateProfile)
{
  std::vector<double> wavelengths = { 200.0, 300.0, 400.0 };
  std::vector<double> temperatures = { 200.0, 300.0 };
  std::vector<std::vector<double>> xs_data = {
    { 1e-18, 2e-18, 3e-18 },
    { 2e-18, 4e-18, 6e-18 }
  };

  BaseCrossSection xs("Test", wavelengths, temperatures, xs_data);

  // Wavelength grid
  GridSpec wl_spec{ "wavelength", "nm", 3 };
  std::vector<double> wl_edges = { 150.0, 250.0, 350.0, 450.0 };
  Grid wl_grid(wl_spec, wl_edges);

  // Altitude grid
  GridSpec alt_spec{ "altitude", "km", 3 };
  std::vector<double> alt_edges = { 0.0, 10.0, 20.0, 30.0 };
  Grid alt_grid(alt_spec, alt_edges);

  // Temperature profile: 280 K at bottom, decreasing with altitude
  ProfileSpec temp_spec{ "temperature", "K", 3 };
  std::vector<double> temp_values = { 280.0, 250.0, 220.0 };
  Profile temp_profile(temp_spec, temp_values);

  auto profile = xs.CalculateProfile(wl_grid, alt_grid, temp_profile);

  ASSERT_EQ(profile.size(), 3u);  // 3 altitude layers
  ASSERT_EQ(profile[0].size(), 3u);  // 3 wavelength bins

  // Layer 0 at 280 K: 80% between 200 K and 300 K
  // xs at 200K = [1e-18, 2e-18, 3e-18], at 300K = [2e-18, 4e-18, 6e-18]
  // Interpolated: [1.8e-18, 3.6e-18, 5.4e-18]
  EXPECT_NEAR(profile[0][0], 1.8e-18, 1e-21);

  // Layer 1 at 250 K: 50% between 200 K and 300 K
  EXPECT_NEAR(profile[1][0], 1.5e-18, 1e-21);

  // Layer 2 at 220 K: 20% between 200 K and 300 K
  EXPECT_NEAR(profile[2][0], 1.2e-18, 1e-21);
}
