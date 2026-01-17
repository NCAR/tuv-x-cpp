#include <tuvx/quantum_yield/quantum_yield.hpp>
#include <tuvx/quantum_yield/types/base.hpp>
#include <tuvx/grid/grid.hpp>
#include <tuvx/profile/profile.hpp>

#include <cmath>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// ============================================================================
// ConstantQuantumYield Tests
// ============================================================================

TEST(ConstantQuantumYieldTest, Construction)
{
  ConstantQuantumYield qy("NO2->NO+O", "NO2", "NO+O", 1.0);

  EXPECT_EQ(qy.Name(), "NO2->NO+O");
  EXPECT_EQ(qy.Reactant(), "NO2");
  EXPECT_EQ(qy.Products(), "NO+O");
  EXPECT_DOUBLE_EQ(qy.Value(), 1.0);
}

TEST(ConstantQuantumYieldTest, CalculateReturnsConstant)
{
  ConstantQuantumYield qy("Test", "A", "B", 0.5);

  GridSpec spec{ "wavelength", "nm", 5 };
  std::vector<double> edges = { 200.0, 250.0, 300.0, 350.0, 400.0, 450.0 };
  Grid wl_grid(spec, edges);

  auto result = qy.Calculate(wl_grid, 298.0);

  ASSERT_EQ(result.size(), 5u);
  for (const auto& val : result)
  {
    EXPECT_DOUBLE_EQ(val, 0.5);
  }
}

TEST(ConstantQuantumYieldTest, CalculateIgnoresTemperature)
{
  ConstantQuantumYield qy("Test", "A", "B", 0.8);

  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 200.0, 300.0, 400.0, 500.0 };
  Grid wl_grid(spec, edges);

  auto result_200K = qy.Calculate(wl_grid, 200.0);
  auto result_300K = qy.Calculate(wl_grid, 300.0);

  ASSERT_EQ(result_200K.size(), result_300K.size());
  for (std::size_t i = 0; i < result_200K.size(); ++i)
  {
    EXPECT_DOUBLE_EQ(result_200K[i], result_300K[i]);
  }
}

TEST(ConstantQuantumYieldTest, CalculateIgnoresAirDensity)
{
  ConstantQuantumYield qy("Test", "A", "B", 0.9);

  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 200.0, 300.0, 400.0, 500.0 };
  Grid wl_grid(spec, edges);

  auto result_lo = qy.Calculate(wl_grid, 298.0, 1e18);
  auto result_hi = qy.Calculate(wl_grid, 298.0, 1e20);

  for (std::size_t i = 0; i < result_lo.size(); ++i)
  {
    EXPECT_DOUBLE_EQ(result_lo[i], result_hi[i]);
  }
}

TEST(ConstantQuantumYieldTest, Clone)
{
  ConstantQuantumYield original("Test", "A", "B", 0.75);
  auto clone = original.Clone();

  EXPECT_EQ(clone->Name(), "Test");
  EXPECT_EQ(clone->Reactant(), "A");
  EXPECT_EQ(clone->Products(), "B");

  ConstantQuantumYield* const_clone = dynamic_cast<ConstantQuantumYield*>(clone.get());
  ASSERT_NE(const_clone, nullptr);
  EXPECT_DOUBLE_EQ(const_clone->Value(), 0.75);
}

// ============================================================================
// BaseQuantumYield Tests
// ============================================================================

TEST(BaseQuantumYieldTest, Construction)
{
  std::vector<double> wavelengths = { 200.0, 300.0, 400.0 };
  std::vector<double> qy_values = { 1.0, 0.5, 0.2 };

  BaseQuantumYield qy("O3->O+O2", "O3", "O+O2", wavelengths, qy_values);

  EXPECT_EQ(qy.Name(), "O3->O+O2");
  EXPECT_EQ(qy.Reactant(), "O3");
  EXPECT_EQ(qy.Products(), "O+O2");
  EXPECT_EQ(qy.ReferenceWavelengths().size(), 3u);
  EXPECT_EQ(qy.ReferenceQuantumYields().size(), 3u);
}

TEST(BaseQuantumYieldTest, CalculateExactWavelengths)
{
  std::vector<double> wavelengths = { 200.0, 300.0, 400.0 };
  std::vector<double> qy_values = { 1.0, 0.5, 0.2 };

  BaseQuantumYield qy("Test", "A", "B", wavelengths, qy_values);

  // Grid with midpoints at reference wavelengths
  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 150.0, 250.0, 350.0, 450.0 };
  Grid wl_grid(spec, edges);

  auto result = qy.Calculate(wl_grid, 298.0);

  ASSERT_EQ(result.size(), 3u);
  EXPECT_NEAR(result[0], 1.0, 1e-10);
  EXPECT_NEAR(result[1], 0.5, 1e-10);
  EXPECT_NEAR(result[2], 0.2, 1e-10);
}

TEST(BaseQuantumYieldTest, CalculateInterpolated)
{
  std::vector<double> wavelengths = { 200.0, 400.0 };
  std::vector<double> qy_values = { 1.0, 0.0 };

  BaseQuantumYield qy("Test", "A", "B", wavelengths, qy_values);

  // Grid with midpoint at 300 nm
  GridSpec spec{ "wavelength", "nm", 1 };
  std::vector<double> edges = { 250.0, 350.0 };
  Grid wl_grid(spec, edges);

  auto result = qy.Calculate(wl_grid, 298.0);

  ASSERT_EQ(result.size(), 1u);
  // Linear interpolation: (300-200)/(400-200) = 0.5 -> 1.0 - 0.5*1.0 = 0.5
  EXPECT_NEAR(result[0], 0.5, 1e-10);
}

TEST(BaseQuantumYieldTest, CalculateBelowRange)
{
  std::vector<double> wavelengths = { 200.0, 300.0 };
  std::vector<double> qy_values = { 1.0, 0.8 };

  BaseQuantumYield qy("Test", "A", "B", wavelengths, qy_values);

  // Grid with midpoints below reference range [200, 300]
  GridSpec spec{ "wavelength", "nm", 1 };
  std::vector<double> edges = { 100.0, 150.0 };  // midpoint = 125 nm
  Grid wl_grid(spec, edges);

  auto result = qy.Calculate(wl_grid, 298.0);

  ASSERT_EQ(result.size(), 1u);
  EXPECT_DOUBLE_EQ(result[0], 0.0);  // 125 nm is below range [200, 300]
}

TEST(BaseQuantumYieldTest, CalculateAboveRange)
{
  std::vector<double> wavelengths = { 200.0, 300.0 };
  std::vector<double> qy_values = { 1.0, 0.8 };

  BaseQuantumYield qy("Test", "A", "B", wavelengths, qy_values);

  // Grid with midpoints above reference range [200, 300]
  GridSpec spec{ "wavelength", "nm", 1 };
  std::vector<double> edges = { 350.0, 450.0 };  // midpoint = 400 nm
  Grid wl_grid(spec, edges);

  auto result = qy.Calculate(wl_grid, 298.0);

  ASSERT_EQ(result.size(), 1u);
  EXPECT_DOUBLE_EQ(result[0], 0.0);  // 400 nm is above range [200, 300]
}

TEST(BaseQuantumYieldTest, CalculateClampsToUnitInterval)
{
  // Data that would extrapolate above 1
  std::vector<double> wavelengths = { 200.0, 300.0, 400.0 };
  std::vector<double> qy_values = { 0.5, 0.9, 0.99 };

  BaseQuantumYield qy("Test", "A", "B", wavelengths, qy_values);

  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 150.0, 250.0, 350.0, 450.0 };
  Grid wl_grid(spec, edges);

  auto result = qy.Calculate(wl_grid, 298.0);

  for (const auto& val : result)
  {
    EXPECT_GE(val, 0.0);
    EXPECT_LE(val, 1.0);
  }
}

TEST(BaseQuantumYieldTest, Clone)
{
  std::vector<double> wavelengths = { 200.0, 300.0 };
  std::vector<double> qy_values = { 1.0, 0.5 };

  BaseQuantumYield original("Test", "A", "B", wavelengths, qy_values);
  auto clone = original.Clone();

  EXPECT_EQ(clone->Name(), "Test");

  BaseQuantumYield* base_clone = dynamic_cast<BaseQuantumYield*>(clone.get());
  ASSERT_NE(base_clone, nullptr);
  EXPECT_EQ(base_clone->ReferenceWavelengths().size(), 2u);
}

// ============================================================================
// QuantumYield Profile Calculation Tests
// ============================================================================

TEST(QuantumYieldTest, CalculateProfile)
{
  ConstantQuantumYield qy("Test", "A", "B", 0.5);

  // Wavelength grid
  GridSpec wl_spec{ "wavelength", "nm", 3 };
  std::vector<double> wl_edges = { 200.0, 300.0, 400.0, 500.0 };
  Grid wl_grid(wl_spec, wl_edges);

  // Altitude grid
  GridSpec alt_spec{ "altitude", "km", 3 };
  std::vector<double> alt_edges = { 0.0, 10.0, 20.0, 30.0 };
  Grid alt_grid(alt_spec, alt_edges);

  // Temperature profile
  ProfileSpec temp_spec{ "temperature", "K", 3 };
  std::vector<double> temp_values = { 288.0, 250.0, 220.0 };
  Profile temp_profile(temp_spec, temp_values);

  // Air density profile
  ProfileSpec dens_spec{ "air_density", "molecules/cm^3", 3 };
  std::vector<double> dens_values = { 2.5e19, 1e19, 5e18 };
  Profile dens_profile(dens_spec, dens_values);

  auto profile = qy.CalculateProfile(wl_grid, alt_grid, temp_profile, dens_profile);

  ASSERT_EQ(profile.size(), 3u);  // 3 altitude layers
  for (const auto& layer : profile)
  {
    ASSERT_EQ(layer.size(), 3u);  // 3 wavelength bins
    for (const auto& val : layer)
    {
      EXPECT_DOUBLE_EQ(val, 0.5);  // Constant value
    }
  }
}

TEST(QuantumYieldTest, CalculateProfileWavelengthDependent)
{
  std::vector<double> wavelengths = { 200.0, 350.0, 500.0 };
  std::vector<double> qy_values = { 1.0, 0.5, 0.1 };

  BaseQuantumYield qy("Test", "A", "B", wavelengths, qy_values);

  // Wavelength grid
  GridSpec wl_spec{ "wavelength", "nm", 3 };
  std::vector<double> wl_edges = { 150.0, 250.0, 400.0, 550.0 };
  Grid wl_grid(wl_spec, wl_edges);

  // Altitude grid
  GridSpec alt_spec{ "altitude", "km", 2 };
  std::vector<double> alt_edges = { 0.0, 15.0, 30.0 };
  Grid alt_grid(alt_spec, alt_edges);

  // Profiles
  ProfileSpec temp_spec{ "temperature", "K", 2 };
  std::vector<double> temp_values = { 288.0, 220.0 };
  Profile temp_profile(temp_spec, temp_values);

  ProfileSpec dens_spec{ "air_density", "molecules/cm^3", 2 };
  std::vector<double> dens_values = { 2.5e19, 5e18 };
  Profile dens_profile(dens_spec, dens_values);

  auto profile = qy.CalculateProfile(wl_grid, alt_grid, temp_profile, dens_profile);

  ASSERT_EQ(profile.size(), 2u);  // 2 altitude layers
  for (const auto& layer : profile)
  {
    ASSERT_EQ(layer.size(), 3u);  // 3 wavelength bins
    // Values should decrease with wavelength
    EXPECT_GT(layer[0], layer[2]);
  }
}
