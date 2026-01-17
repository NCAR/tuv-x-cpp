#include <tuvx/cross_section/types/base.hpp>
#include <tuvx/photolysis/photolysis_rate.hpp>
#include <tuvx/quantum_yield/types/base.hpp>

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// Helper to create a simple wavelength grid
Grid CreateWavelengthGrid(const std::vector<double>& edges)
{
  GridSpec spec{ "wavelength", "nm", edges.size() - 1 };
  return Grid(spec, edges);
}

// Helper to create uniform radiation field
RadiationField CreateUniformField(std::size_t n_levels, std::size_t n_wavelengths, double flux)
{
  RadiationField field;
  field.Initialize(n_levels, n_wavelengths);

  for (std::size_t i = 0; i < n_levels; ++i)
  {
    for (std::size_t j = 0; j < n_wavelengths; ++j)
    {
      field.actinic_flux_direct[i][j] = flux;
      field.actinic_flux_diffuse[i][j] = 0.0;
    }
  }

  return field;
}

// ============================================================================
// PhotolysisRateCalculator Construction Tests
// ============================================================================

TEST(PhotolysisRateTest, Construction)
{
  std::vector<double> wl = { 300.0, 400.0 };
  std::vector<double> xs = { 1e-18, 1e-18 };
  BaseCrossSection cross_section("O3", wl, xs);

  ConstantQuantumYield quantum_yield("O3->O2+O", "O3", "O2+O", 1.0);

  PhotolysisRateCalculator calc("O3 -> O2 + O(1D)", &cross_section, &quantum_yield);

  EXPECT_EQ(calc.ReactionName(), "O3 -> O2 + O(1D)");
}

// ============================================================================
// Empty/Null Input Tests
// ============================================================================

TEST(PhotolysisRateTest, EmptyRadiationField)
{
  std::vector<double> wl = { 300.0, 400.0 };
  std::vector<double> xs = { 1e-18, 1e-18 };
  BaseCrossSection cross_section("O3", wl, xs);
  ConstantQuantumYield quantum_yield("O3 photolysis", "O3", "O2+O", 1.0);

  PhotolysisRateCalculator calc("O3 photolysis", &cross_section, &quantum_yield);

  RadiationField empty_field;
  auto grid = CreateWavelengthGrid({ 290.0, 350.0, 410.0 });

  auto result = calc.Calculate(empty_field, grid);

  EXPECT_TRUE(result.rates.empty());
}

TEST(PhotolysisRateTest, NullCrossSection)
{
  ConstantQuantumYield quantum_yield("test", "X", "products", 1.0);

  PhotolysisRateCalculator calc("test", nullptr, &quantum_yield);

  auto field = CreateUniformField(3, 2, 1e15);
  auto grid = CreateWavelengthGrid({ 300.0, 350.0, 400.0 });

  auto result = calc.Calculate(field, grid);

  EXPECT_TRUE(result.rates.empty());
}

TEST(PhotolysisRateTest, NullQuantumYield)
{
  std::vector<double> wl = { 300.0, 400.0 };
  std::vector<double> xs = { 1e-18, 1e-18 };
  BaseCrossSection cross_section("test", wl, xs);

  PhotolysisRateCalculator calc("test", &cross_section, nullptr);

  auto field = CreateUniformField(3, 2, 1e15);
  auto grid = CreateWavelengthGrid({ 300.0, 350.0, 400.0 });

  auto result = calc.Calculate(field, grid);

  EXPECT_TRUE(result.rates.empty());
}

// ============================================================================
// Basic Calculation Tests
// ============================================================================

TEST(PhotolysisRateTest, SimpleCalculation)
{
  // Simple case: constant cross-section, constant quantum yield, uniform flux
  // J = F × σ × φ × Δλ

  std::vector<double> wl = { 300.0, 400.0 };
  std::vector<double> xs = { 1e-18, 1e-18 };  // 1e-18 cm^2
  BaseCrossSection cross_section("test", wl, xs);

  ConstantQuantumYield quantum_yield("test", "X", "products", 1.0);

  PhotolysisRateCalculator calc("test reaction", &cross_section, &quantum_yield);

  // Create uniform actinic flux of 1e15 photons/cm^2/s
  auto field = CreateUniformField(2, 1, 1e15);

  // Wavelength bin: 300-400 nm (100 nm width, midpoint 350 nm)
  auto grid = CreateWavelengthGrid({ 300.0, 400.0 });

  auto result = calc.Calculate(field, grid);

  ASSERT_EQ(result.NumberOfLevels(), 2u);

  // Expected J = 1e15 × 1e-18 × 1.0 × 100 = 1e-1 s^-1
  double expected = 1e15 * 1e-18 * 1.0 * 100.0;
  EXPECT_NEAR(result.rates[0], expected, expected * 0.1);
  EXPECT_NEAR(result.rates[1], expected, expected * 0.1);
}

TEST(PhotolysisRateTest, CalculateAtLevel)
{
  std::vector<double> wl = { 300.0, 400.0 };
  std::vector<double> xs = { 1e-18, 1e-18 };
  BaseCrossSection cross_section("test", wl, xs);
  ConstantQuantumYield quantum_yield("test", "X", "products", 1.0);

  PhotolysisRateCalculator calc("test", &cross_section, &quantum_yield);

  auto grid = CreateWavelengthGrid({ 300.0, 400.0 });
  std::vector<double> actinic_flux = { 1e15 };

  double j = calc.CalculateAtLevel(actinic_flux, grid);

  double expected = 1e15 * 1e-18 * 1.0 * 100.0;
  EXPECT_NEAR(j, expected, expected * 0.1);
}

TEST(PhotolysisRateTest, MultipleWavelengthBins)
{
  // Cross-section varies with wavelength
  std::vector<double> wl = { 290.0, 310.0, 330.0, 350.0 };
  std::vector<double> xs = { 1e-17, 1e-18, 1e-19, 1e-20 };  // Decreasing
  BaseCrossSection cross_section("test", wl, xs);

  ConstantQuantumYield quantum_yield("test", "X", "products", 1.0);

  PhotolysisRateCalculator calc("test", &cross_section, &quantum_yield);

  // 3 wavelength bins, uniform flux
  auto field = CreateUniformField(2, 3, 1e15);
  auto grid = CreateWavelengthGrid({ 290.0, 310.0, 330.0, 350.0 });  // 20 nm bins

  auto result = calc.Calculate(field, grid);

  // Each bin contributes: F × σ × φ × 20
  // Total should be sum of contributions from each bin
  ASSERT_EQ(result.NumberOfLevels(), 2u);
  EXPECT_GT(result.rates[0], 0.0);
}

// ============================================================================
// Physical Scenario Tests
// ============================================================================

TEST(PhotolysisRateTest, VerticalProfile)
{
  // Actinic flux decreases with depth - rates should decrease too
  std::vector<double> wl = { 300.0, 400.0 };
  std::vector<double> xs = { 1e-18, 1e-18 };
  BaseCrossSection cross_section("test", wl, xs);
  ConstantQuantumYield quantum_yield("test", "X", "products", 1.0);

  PhotolysisRateCalculator calc("test", &cross_section, &quantum_yield);

  RadiationField field;
  field.Initialize(4, 1);  // 4 levels, 1 wavelength

  // Flux decreasing from TOA to surface
  field.actinic_flux_direct[3][0] = 1e15;   // TOA
  field.actinic_flux_direct[2][0] = 5e14;
  field.actinic_flux_direct[1][0] = 2e14;
  field.actinic_flux_direct[0][0] = 1e14;   // Surface

  auto grid = CreateWavelengthGrid({ 300.0, 400.0 });
  auto result = calc.Calculate(field, grid);

  // Rates should decrease from TOA to surface
  EXPECT_GT(result.rates[3], result.rates[2]);
  EXPECT_GT(result.rates[2], result.rates[1]);
  EXPECT_GT(result.rates[1], result.rates[0]);
}

TEST(PhotolysisRateTest, SurfaceRate)
{
  std::vector<double> wl = { 300.0, 400.0 };
  std::vector<double> xs = { 1e-18, 1e-18 };
  BaseCrossSection cross_section("test", wl, xs);
  ConstantQuantumYield quantum_yield("test", "X", "products", 1.0);

  PhotolysisRateCalculator calc("test", &cross_section, &quantum_yield);

  auto field = CreateUniformField(3, 1, 1e15);
  auto grid = CreateWavelengthGrid({ 300.0, 400.0 });

  auto result = calc.Calculate(field, grid);

  EXPECT_EQ(result.SurfaceRate(), result.rates[0]);
}

// ============================================================================
// PhotolysisRateSet Tests
// ============================================================================

TEST(PhotolysisRateSetTest, Construction)
{
  PhotolysisRateSet set;

  EXPECT_TRUE(set.Empty());
  EXPECT_EQ(set.Size(), 0u);
}

TEST(PhotolysisRateSetTest, AddReaction)
{
  std::vector<double> wl = { 300.0, 400.0 };
  std::vector<double> xs = { 1e-18, 1e-18 };
  BaseCrossSection cross_section("O3", wl, xs);
  ConstantQuantumYield quantum_yield("O3 photolysis", "O3", "O2+O", 1.0);

  PhotolysisRateSet set;
  set.AddReaction("O3 -> O2 + O(1D)", &cross_section, &quantum_yield);

  EXPECT_FALSE(set.Empty());
  EXPECT_EQ(set.Size(), 1u);

  auto names = set.ReactionNames();
  ASSERT_EQ(names.size(), 1u);
  EXPECT_EQ(names[0], "O3 -> O2 + O(1D)");
}

TEST(PhotolysisRateSetTest, MultipleReactions)
{
  std::vector<double> wl = { 300.0, 400.0 };
  std::vector<double> xs1 = { 1e-18, 1e-18 };
  std::vector<double> xs2 = { 5e-19, 5e-19 };

  BaseCrossSection xs_o3("O3", wl, xs1);
  BaseCrossSection xs_no2("NO2", wl, xs2);
  ConstantQuantumYield qy_o3("O3 photolysis", "O3", "O2+O", 1.0);
  ConstantQuantumYield qy_no2("NO2 photolysis", "NO2", "NO+O", 0.8);

  PhotolysisRateSet set;
  set.AddReaction("O3 -> O2 + O(1D)", &xs_o3, &qy_o3);
  set.AddReaction("NO2 -> NO + O", &xs_no2, &qy_no2);

  EXPECT_EQ(set.Size(), 2u);

  auto names = set.ReactionNames();
  EXPECT_EQ(names[0], "O3 -> O2 + O(1D)");
  EXPECT_EQ(names[1], "NO2 -> NO + O");
}

TEST(PhotolysisRateSetTest, CalculateAll)
{
  std::vector<double> wl = { 300.0, 400.0 };
  std::vector<double> xs1 = { 1e-18, 1e-18 };
  std::vector<double> xs2 = { 5e-19, 5e-19 };

  BaseCrossSection xs_o3("O3", wl, xs1);
  BaseCrossSection xs_no2("NO2", wl, xs2);
  ConstantQuantumYield qy_o3("O3 photolysis", "O3", "O2+O", 1.0);
  ConstantQuantumYield qy_no2("NO2 photolysis", "NO2", "NO+O", 1.0);

  PhotolysisRateSet set;
  set.AddReaction("O3 photolysis", &xs_o3, &qy_o3);
  set.AddReaction("NO2 photolysis", &xs_no2, &qy_no2);

  auto field = CreateUniformField(2, 1, 1e15);
  auto grid = CreateWavelengthGrid({ 300.0, 400.0 });

  auto results = set.CalculateAll(field, grid);

  ASSERT_EQ(results.size(), 2u);
  EXPECT_EQ(results[0].reaction_name, "O3 photolysis");
  EXPECT_EQ(results[1].reaction_name, "NO2 photolysis");

  // O3 should have higher rate (larger cross-section)
  EXPECT_GT(results[0].rates[0], results[1].rates[0]);
}

// ============================================================================
// Realistic Scenario Tests
// ============================================================================

TEST(PhotolysisRateTest, O3PhotolysisRealistic)
{
  // Realistic O3 photolysis: O3 + hν -> O2 + O(1D)
  // Typical values in Hartley band (200-320 nm)

  std::vector<double> wl = { 280.0, 290.0, 300.0, 310.0, 320.0 };
  // O3 cross-section peaks around 255 nm, decreases toward 320 nm
  std::vector<double> xs = { 5e-19, 2e-19, 5e-20, 1e-20, 2e-21 };
  BaseCrossSection cross_section("O3", wl, xs);

  // O(1D) quantum yield is high at short wavelengths
  ConstantQuantumYield quantum_yield("O3->O1D", "O3", "O2+O1D", 0.9);

  PhotolysisRateCalculator calc("O3 -> O2 + O(1D)", &cross_section, &quantum_yield);

  // Typical stratospheric actinic flux
  RadiationField field;
  field.Initialize(2, 4);
  for (std::size_t j = 0; j < 4; ++j)
  {
    field.actinic_flux_direct[0][j] = 1e14;  // Surface (reduced)
    field.actinic_flux_direct[1][j] = 1e15;  // TOA
  }

  auto grid = CreateWavelengthGrid({ 280.0, 290.0, 300.0, 310.0, 320.0 });
  auto result = calc.Calculate(field, grid);

  // J(O1D) typically 1e-5 to 1e-4 s^-1 at surface, higher aloft
  EXPECT_GT(result.rates[0], 0.0);
  EXPECT_GT(result.rates[1], result.rates[0]);

  // Order of magnitude check: J should be reasonable
  EXPECT_GT(result.rates[1], 1e-10);
  EXPECT_LT(result.rates[1], 1e0);
}

TEST(PhotolysisRateTest, WavelengthDependence)
{
  // Photolysis should be dominated by wavelengths with high σ × F product

  // Cross-section decreasing with wavelength
  std::vector<double> wl = { 300.0, 350.0, 400.0 };
  std::vector<double> xs = { 1e-17, 1e-19, 1e-21 };  // Decreasing 100x per step
  BaseCrossSection cross_section("test", wl, xs);
  ConstantQuantumYield quantum_yield("test", "X", "products", 1.0);

  PhotolysisRateCalculator calc("test", &cross_section, &quantum_yield);

  // Uniform flux
  auto field = CreateUniformField(2, 2, 1e15);
  auto grid = CreateWavelengthGrid({ 300.0, 350.0, 400.0 });  // 50 nm bins

  auto result = calc.Calculate(field, grid);

  // Rate should be dominated by short wavelength bin
  // Short: 1e15 * 1e-17 * 50 = 5e-1
  // Long: 1e15 * 1e-21 * 50 = 5e-5

  EXPECT_GT(result.rates[0], 0.0);
}
