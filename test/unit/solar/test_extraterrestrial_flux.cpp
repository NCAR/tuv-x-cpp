#include <tuvx/solar/extraterrestrial_flux.hpp>

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;
using namespace tuvx::solar;

// ============================================================================
// Unit Conversion Tests
// ============================================================================

TEST(IrradianceConversionTest, RoundTrip)
{
  double wavelength = 500.0;  // nm
  double irradiance = 1.9;    // W/m^2/nm (typical at 500 nm)

  double photon_flux = IrradianceToPhotonFlux(irradiance, wavelength);
  double back_irradiance = PhotonFluxToIrradiance(photon_flux, wavelength);

  EXPECT_NEAR(back_irradiance, irradiance, irradiance * 1e-10);
}

TEST(IrradianceConversionTest, WavelengthDependence)
{
  double irradiance = 1.0;  // W/m^2/nm

  // Longer wavelength = more photons per watt
  double flux_300 = IrradianceToPhotonFlux(irradiance, 300.0);
  double flux_600 = IrradianceToPhotonFlux(irradiance, 600.0);

  EXPECT_GT(flux_600, flux_300);
  // Should be approximately 2x (ratio of wavelengths)
  EXPECT_NEAR(flux_600 / flux_300, 2.0, 0.01);
}

TEST(IrradianceConversionTest, OrderOfMagnitude)
{
  // At 500 nm, solar irradiance ~1.9 W/m^2/nm
  // Photon energy E = hc/λ = 6.63e-34 * 3e8 / (500e-9) ≈ 4e-19 J
  // Photons/J ≈ 2.5e18
  // 1.9 W/m^2 * 1e-4 m^2/cm^2 * 2.5e18 ≈ 4.75e14 photons/cm^2/s/nm

  double flux = IrradianceToPhotonFlux(1.9, 500.0);
  EXPECT_GT(flux, 1e14);
  EXPECT_LT(flux, 1e15);
}

// ============================================================================
// ExtraterrestrialFlux Construction Tests
// ============================================================================

TEST(ExtraterrestrialFluxTest, Construction)
{
  std::vector<double> wavelengths = { 300.0, 400.0, 500.0 };
  std::vector<double> flux = { 1e14, 2e14, 1.5e14 };

  ExtraterrestrialFlux etr(wavelengths, flux);

  EXPECT_EQ(etr.ReferenceWavelengths().size(), 3u);
  EXPECT_EQ(etr.ReferenceFlux().size(), 3u);
}

TEST(ExtraterrestrialFluxTest, ConstructionMismatchedSizes)
{
  std::vector<double> wavelengths = { 300.0, 400.0, 500.0 };
  std::vector<double> flux = { 1e14, 2e14 };  // Wrong size

  EXPECT_THROW(ExtraterrestrialFlux(wavelengths, flux), std::invalid_argument);
}

TEST(ExtraterrestrialFluxTest, ConstructionTooFewPoints)
{
  std::vector<double> wavelengths = { 300.0 };
  std::vector<double> flux = { 1e14 };

  EXPECT_THROW(ExtraterrestrialFlux(wavelengths, flux), std::invalid_argument);
}

// ============================================================================
// ExtraterrestrialFlux Calculate Tests
// ============================================================================

TEST(ExtraterrestrialFluxTest, CalculateOnGrid)
{
  std::vector<double> ref_wavelengths = { 300.0, 400.0, 500.0 };
  std::vector<double> ref_flux = { 1e14, 2e14, 1.5e14 };
  ExtraterrestrialFlux etr(ref_wavelengths, ref_flux);

  // Create a grid that spans the reference range
  GridSpec spec{ "wavelength", "nm", 2 };
  std::vector<double> edges = { 300.0, 400.0, 500.0 };
  Grid grid(spec, edges);

  auto result = etr.Calculate(grid);

  ASSERT_EQ(result.size(), 2u);
  // Midpoints are 350 and 450 nm
  // At 350: interpolate between 300 (1e14) and 400 (2e14) -> 1.5e14
  EXPECT_NEAR(result[0], 1.5e14, 1e12);
  // At 450: interpolate between 400 (2e14) and 500 (1.5e14) -> 1.75e14
  EXPECT_NEAR(result[1], 1.75e14, 1e12);
}

TEST(ExtraterrestrialFluxTest, CalculateOutsideRange)
{
  std::vector<double> ref_wavelengths = { 300.0, 400.0, 500.0 };
  std::vector<double> ref_flux = { 1e14, 2e14, 1.5e14 };
  ExtraterrestrialFlux etr(ref_wavelengths, ref_flux);

  // Grid extends beyond reference range
  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 200.0, 280.0, 400.0, 700.0 };  // Midpoints: 240, 340, 550
  Grid grid(spec, edges);

  auto result = etr.Calculate(grid);

  ASSERT_EQ(result.size(), 3u);
  EXPECT_DOUBLE_EQ(result[0], 0.0);  // 240 nm is below range (300)
  EXPECT_GT(result[1], 0.0);         // 340 nm is in range
  EXPECT_DOUBLE_EQ(result[2], 0.0);  // 550 nm midpoint is above range (500)
}

TEST(ExtraterrestrialFluxTest, DistanceCorrection)
{
  std::vector<double> ref_wavelengths = { 300.0, 500.0 };
  std::vector<double> ref_flux = { 1e14, 1e14 };
  ExtraterrestrialFlux etr(ref_wavelengths, ref_flux);

  GridSpec spec{ "wavelength", "nm", 1 };
  std::vector<double> edges = { 300.0, 500.0 };
  Grid grid(spec, edges);

  // At 1 AU
  auto flux_1au = etr.Calculate(grid, 1.0);

  // At perihelion (closer, factor > 1)
  auto flux_perihelion = etr.Calculate(grid, 1.034);

  // At aphelion (farther, factor < 1)
  auto flux_aphelion = etr.Calculate(grid, 0.967);

  EXPECT_GT(flux_perihelion[0], flux_1au[0]);
  EXPECT_LT(flux_aphelion[0], flux_1au[0]);
}

TEST(ExtraterrestrialFluxTest, CalculateIntegrated)
{
  std::vector<double> ref_wavelengths = { 300.0, 500.0 };
  std::vector<double> ref_flux = { 1e14, 1e14 };  // Constant flux
  ExtraterrestrialFlux etr(ref_wavelengths, ref_flux);

  GridSpec spec{ "wavelength", "nm", 2 };
  std::vector<double> edges = { 300.0, 400.0, 500.0 };  // 100 nm bins
  Grid grid(spec, edges);

  auto integrated = etr.CalculateIntegrated(grid);

  ASSERT_EQ(integrated.size(), 2u);
  // Spectral flux ~1e14 photons/cm^2/s/nm * 100 nm = 1e16 photons/cm^2/s
  EXPECT_NEAR(integrated[0], 1e16, 1e14);
  EXPECT_NEAR(integrated[1], 1e16, 1e14);
}

// ============================================================================
// Blackbody Spectrum Tests
// ============================================================================

TEST(BlackbodySolarFluxTest, PeakWavelength)
{
  // Wien's law for photon flux: λ_peak ≈ 635 nm for T = 5778 K
  // (Photon peak is at longer wavelength than energy peak due to E = hc/λ)
  std::vector<double> wavelengths = { 400.0, 450.0, 500.0, 550.0, 600.0, 650.0, 700.0, 750.0 };

  auto flux = BlackbodySolarFlux(wavelengths, 5778.0);

  // Find peak
  std::size_t peak_idx = 0;
  for (std::size_t i = 1; i < flux.size(); ++i)
  {
    if (flux[i] > flux[peak_idx])
    {
      peak_idx = i;
    }
  }

  // Peak should be in the 500-700 nm range for photon flux
  EXPECT_GE(peak_idx, 2u);
  EXPECT_LE(peak_idx, 6u);
}

TEST(BlackbodySolarFluxTest, PositiveValues)
{
  std::vector<double> wavelengths = { 300.0, 400.0, 500.0, 600.0, 700.0 };

  auto flux = BlackbodySolarFlux(wavelengths);

  for (const auto& f : flux)
  {
    EXPECT_GT(f, 0.0);
  }
}

TEST(BlackbodySolarFluxTest, UVLowerThanVisible)
{
  std::vector<double> wavelengths = { 280.0, 500.0 };

  auto flux = BlackbodySolarFlux(wavelengths);

  // UV flux should be lower than visible (below peak)
  EXPECT_LT(flux[0], flux[1]);
}

// ============================================================================
// Reference Spectrum Tests
// ============================================================================

TEST(ReferenceSpectraTest, ASTM_E490_Construction)
{
  auto [wavelengths, flux] = reference_spectra::ASTM_E490_AM0_Simplified();

  EXPECT_GT(wavelengths.size(), 10u);
  EXPECT_EQ(wavelengths.size(), flux.size());

  // All values should be positive
  for (const auto& f : flux)
  {
    EXPECT_GT(f, 0.0);
  }
}

TEST(ReferenceSpectraTest, ASTM_E490_CreateETR)
{
  auto etr = reference_spectra::CreateASTM_E490();

  EXPECT_GT(etr.ReferenceWavelengths().size(), 10u);

  // Test that we can calculate on a grid
  GridSpec spec{ "wavelength", "nm", 3 };
  std::vector<double> edges = { 300.0, 400.0, 500.0, 600.0 };
  Grid grid(spec, edges);

  auto flux = etr.Calculate(grid);
  ASSERT_EQ(flux.size(), 3u);

  for (const auto& f : flux)
  {
    EXPECT_GT(f, 0.0);
  }
}

TEST(ReferenceSpectraTest, ASTM_E490_PhysicalRange)
{
  auto etr = reference_spectra::CreateASTM_E490();

  // Create a visible light grid
  GridSpec spec{ "wavelength", "nm", 1 };
  std::vector<double> edges = { 400.0, 700.0 };
  Grid grid(spec, edges);

  auto flux = etr.Calculate(grid);

  // Visible flux should be substantial (order of 1e15 photons/cm^2/s/nm)
  EXPECT_GT(flux[0], 1e14);
  EXPECT_LT(flux[0], 1e16);
}

// ============================================================================
// Integration with Solar Position Tests
// ============================================================================

TEST(ExtraterrestrialFluxTest, WithEarthSunDistance)
{
  auto etr = reference_spectra::CreateASTM_E490();

  GridSpec spec{ "wavelength", "nm", 1 };
  std::vector<double> edges = { 400.0, 600.0 };
  Grid grid(spec, edges);

  // Get flux at perihelion and aphelion
  int perihelion_day = 3;    // ~Jan 3
  int aphelion_day = 185;    // ~Jul 4

  double factor_perihelion = EarthSunDistanceFactor(perihelion_day);
  double factor_aphelion = EarthSunDistanceFactor(aphelion_day);

  auto flux_perihelion = etr.Calculate(grid, factor_perihelion);
  auto flux_aphelion = etr.Calculate(grid, factor_aphelion);

  // Perihelion flux should be ~7% higher than aphelion
  double ratio = flux_perihelion[0] / flux_aphelion[0];
  EXPECT_NEAR(ratio, 1.07, 0.02);
}
