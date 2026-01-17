/// @file test_spectral_analysis.cpp
/// @brief Generate spectral data for radiator visualization
///
/// Outputs CSV files for plotting:
/// 1. Cross-section spectra (Rayleigh, O3, O2)
/// 2. Optical depth by radiator type vs wavelength
/// 3. Altitude-resolved actinic flux spectra

#include <gtest/gtest.h>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <tuvx/cross_section/types/base.hpp>
#include <tuvx/cross_section/types/o2.hpp>
#include <tuvx/cross_section/types/o3.hpp>
#include <tuvx/grid/grid.hpp>
#include <tuvx/model/model_config.hpp>
#include <tuvx/model/tuv_model.hpp>
#include <tuvx/radiator/types/aerosol.hpp>
#include <tuvx/radiator/types/from_cross_section.hpp>
#include <tuvx/radiator/types/rayleigh.hpp>

namespace
{
  const std::string kOutputDir = "/Users/fillmore/EarthSystem/TUV-x-cpp/plots/";

  /// Helper to create wavelength grid
  std::vector<double> CreateWavelengthMidpoints(double min_nm, double max_nm, std::size_t n_points)
  {
    std::vector<double> wavelengths(n_points);
    double step = (max_nm - min_nm) / (n_points - 1);
    for (std::size_t i = 0; i < n_points; ++i)
    {
      wavelengths[i] = min_nm + i * step;
    }
    return wavelengths;
  }
}  // namespace

/// =============================================================================
/// Test 1: Cross-Section Spectra
/// =============================================================================
/// Outputs cross-sections for Rayleigh, O3, and O2 vs wavelength
/// Shows the characteristic spectral shapes of each process

TEST(SpectralAnalysis, CrossSectionSpectra)
{
  // Wide wavelength range to show all features
  auto wavelengths = CreateWavelengthMidpoints(150.0, 700.0, 551);

  // Create wavelength grid for cross-section calculations
  std::size_t n_bins = wavelengths.size();
  double dw = (wavelengths.back() - wavelengths.front()) / (n_bins - 1);

  std::vector<double> wl_edges(n_bins + 1);
  for (std::size_t i = 0; i <= n_bins; ++i)
  {
    wl_edges[i] = wavelengths.front() - dw / 2 + i * dw;
  }

  tuvx::GridSpec wl_spec{ "wavelength", "nm", n_bins };
  tuvx::Grid wl_grid(wl_spec, wl_edges);

  // Calculate cross-sections at 250 K (stratospheric temperature)
  double temperature = 250.0;

  tuvx::O3CrossSection o3_xs;
  tuvx::O2CrossSection o2_xs;

  auto o3_sigma = o3_xs.Calculate(wl_grid, temperature);
  auto o2_sigma = o2_xs.Calculate(wl_grid, temperature);

  // Get wavelength midpoints from the grid
  auto grid_midpoints = wl_grid.Midpoints();

  // Write CSV
  std::ofstream file(kOutputDir + "cross_section_spectra.csv");
  file << std::scientific << std::setprecision(6);
  file << "wavelength_nm,rayleigh_cm2,o3_cm2,o2_cm2\n";

  for (std::size_t i = 0; i < n_bins; ++i)
  {
    double rayleigh = tuvx::RayleighRadiator::RayleighCrossSection(grid_midpoints[i]);
    file << grid_midpoints[i] << "," << rayleigh << "," << o3_sigma[i] << "," << o2_sigma[i] << "\n";
  }

  file.close();
  std::cout << "\nWrote: " << kOutputDir << "cross_section_spectra.csv\n";
  std::cout << "Columns: wavelength_nm, rayleigh_cm2, o3_cm2, o2_cm2\n";
  std::cout << "Range: 150-700 nm, " << n_bins << " points\n";

  // Verify expected spectral features
  // Rayleigh should decrease with wavelength (lambda^-4)
  double rayleigh_300 = tuvx::RayleighRadiator::RayleighCrossSection(300.0);
  double rayleigh_600 = tuvx::RayleighRadiator::RayleighCrossSection(600.0);
  EXPECT_GT(rayleigh_300, rayleigh_600 * 10);  // ~16x larger at half wavelength

  EXPECT_TRUE(true);
}

/// =============================================================================
/// Test 2: Optical Depth by Radiator Type
/// =============================================================================
/// Shows total column optical depth from each radiator vs wavelength
/// Reveals which process dominates at each wavelength

TEST(SpectralAnalysis, OpticalDepthByRadiator)
{
  // Configure model
  tuvx::ModelConfig config;
  config.wavelength_min = 200.0;
  config.wavelength_max = 700.0;
  config.n_wavelength_bins = 250;
  config.altitude_min = 0.0;
  config.altitude_max = 80.0;
  config.n_altitude_layers = 80;
  config.solar_zenith_angle = 0.0;  // Overhead sun for simplest geometry
  config.ozone_column_DU = 300.0;

  tuvx::TuvModel model(config);
  model.UseStandardAtmosphere();

  // Get grids and profiles
  const auto grids = model.GetGridWarehouse();
  auto profiles = model.CreateProfileWarehouse();

  const auto& wl_grid = grids.Get("wavelength", "nm");
  const auto& alt_grid = grids.Get("altitude", "km");

  auto wavelengths = wl_grid.Midpoints();
  std::size_t n_wavelengths = wl_grid.Spec().n_cells;
  std::size_t n_layers = alt_grid.Spec().n_cells;

  // Create each radiator and compute optical depths
  tuvx::FromCrossSectionRadiator o3_radiator(
      "O3",
      std::make_unique<tuvx::O3CrossSection>(),
      "O3",          // density profile name
      "temperature", // temperature profile name
      "wavelength",  // wavelength grid name
      "altitude"     // altitude grid name
  );
  tuvx::RayleighRadiator rayleigh_radiator;
  tuvx::AerosolRadiator aerosol_radiator;

  o3_radiator.UpdateState(grids, profiles);
  rayleigh_radiator.UpdateState(grids, profiles);
  aerosol_radiator.UpdateState(grids, profiles);

  const auto& o3_state = o3_radiator.State();
  const auto& rayleigh_state = rayleigh_radiator.State();
  const auto& aerosol_state = aerosol_radiator.State();

  // Sum optical depth over all layers for each wavelength
  std::vector<double> o3_tau(n_wavelengths, 0.0);
  std::vector<double> rayleigh_tau(n_wavelengths, 0.0);
  std::vector<double> aerosol_tau(n_wavelengths, 0.0);

  for (std::size_t j = 0; j < n_wavelengths; ++j)
  {
    for (std::size_t i = 0; i < n_layers; ++i)
    {
      o3_tau[j] += o3_state.optical_depth[i][j];
      rayleigh_tau[j] += rayleigh_state.optical_depth[i][j];
      aerosol_tau[j] += aerosol_state.optical_depth[i][j];
    }
  }

  // Write CSV
  std::ofstream file(kOutputDir + "optical_depth_by_radiator.csv");
  file << std::scientific << std::setprecision(6);
  file << "wavelength_nm,o3_tau,rayleigh_tau,aerosol_tau,total_tau\n";

  for (std::size_t j = 0; j < n_wavelengths; ++j)
  {
    double total = o3_tau[j] + rayleigh_tau[j] + aerosol_tau[j];
    file << wavelengths[j] << "," << o3_tau[j] << "," << rayleigh_tau[j] << "," << aerosol_tau[j]
         << "," << total << "\n";
  }

  file.close();
  std::cout << "\nWrote: " << kOutputDir << "optical_depth_by_radiator.csv\n";
  std::cout << "Columns: wavelength_nm, o3_tau, rayleigh_tau, aerosol_tau, total_tau\n";
  std::cout << "Range: 200-700 nm, " << n_wavelengths << " points\n";
  std::cout << "Atmosphere: 0-80 km, 300 DU O3\n";

  // Verify physical expectations (relaxed for simplified cross-section data)
  std::size_t idx_300 = 50;   // ~300 nm
  std::size_t idx_500 = 150;  // ~500 nm

  // Rayleigh should decrease with wavelength (lambda^-4)
  EXPECT_GT(rayleigh_tau[idx_300], rayleigh_tau[idx_500]);

  // O3 should have some absorption at UV wavelengths
  EXPECT_GT(o3_tau[idx_300], 0.0);

  EXPECT_TRUE(true);
}

/// =============================================================================
/// Test 3: Altitude-Resolved Actinic Flux Spectra
/// =============================================================================
/// Shows how the solar spectrum is modified at different altitudes
/// Demonstrates UV absorption by ozone layer

TEST(SpectralAnalysis, AltitudeResolvedFluxSpectra)
{
  // Configure model with good spectral resolution
  tuvx::ModelConfig config;
  config.wavelength_min = 280.0;  // UV-B to visible
  config.wavelength_max = 700.0;
  config.n_wavelength_bins = 210;
  config.altitude_min = 0.0;
  config.altitude_max = 80.0;
  config.n_altitude_layers = 80;
  config.solar_zenith_angle = 30.0;  // Typical daytime
  config.ozone_column_DU = 300.0;

  tuvx::TuvModel model(config);
  model.UseStandardAtmosphere();
  model.AddStandardRadiators();  // O3 + O2 + Rayleigh

  auto output = model.Calculate();

  const auto& wl_grid = model.WavelengthGrid();
  const auto& alt_grid = model.AltitudeGrid();

  auto wavelengths = wl_grid.Midpoints();
  auto altitudes = alt_grid.Midpoints();

  std::size_t n_wavelengths = wavelengths.size();
  std::size_t n_layers = altitudes.size();

  // Select altitude levels for output
  // TOA (~80 km), upper stratosphere (~50 km), ozone layer (~25 km),
  // lower stratosphere (~15 km), troposphere (~5 km), surface (~0.5 km)
  std::vector<std::size_t> level_indices;
  std::vector<double> level_altitudes;

  // Find indices closest to desired altitudes
  std::vector<double> target_alts = { 79.0, 50.0, 25.0, 15.0, 5.0, 0.5 };
  for (double target : target_alts)
  {
    std::size_t best_idx = 0;
    double best_diff = std::abs(altitudes[0] - target);
    for (std::size_t i = 1; i < n_layers; ++i)
    {
      double diff = std::abs(altitudes[i] - target);
      if (diff < best_diff)
      {
        best_diff = diff;
        best_idx = i;
      }
    }
    level_indices.push_back(best_idx);
    level_altitudes.push_back(altitudes[best_idx]);
  }

  // Write CSV with flux at each altitude level
  std::ofstream file(kOutputDir + "altitude_flux_spectra.csv");
  file << std::scientific << std::setprecision(6);

  // Header with altitude values
  file << "wavelength_nm";
  for (double alt : level_altitudes)
  {
    file << ",flux_" << std::fixed << std::setprecision(1) << alt << "km";
  }
  file << "\n";

  file << std::scientific << std::setprecision(6);

  // Get actinic flux at each level
  std::vector<std::vector<double>> fluxes_at_levels;
  for (std::size_t idx : level_indices)
  {
    fluxes_at_levels.push_back(output.GetActinicFlux(idx));
  }

  for (std::size_t j = 0; j < n_wavelengths; ++j)
  {
    file << wavelengths[j];
    for (const auto& flux : fluxes_at_levels)
    {
      file << "," << flux[j];
    }
    file << "\n";
  }

  file.close();
  std::cout << "\nWrote: " << kOutputDir << "altitude_flux_spectra.csv\n";
  std::cout << "Columns: wavelength_nm, flux at ";
  for (std::size_t i = 0; i < level_altitudes.size(); ++i)
  {
    std::cout << level_altitudes[i] << " km";
    if (i < level_altitudes.size() - 1)
      std::cout << ", ";
  }
  std::cout << "\n";
  std::cout << "SZA: 30 deg, O3 column: 300 DU\n";

  // Verify physical expectations
  // UV-B (300 nm) should be heavily attenuated at surface vs TOA
  std::size_t idx_300 = 10;   // ~300 nm
  std::size_t idx_500 = 110;  // ~500 nm

  double flux_300_toa = fluxes_at_levels[0][idx_300];
  double flux_300_sfc = fluxes_at_levels.back()[idx_300];
  double flux_500_toa = fluxes_at_levels[0][idx_500];
  double flux_500_sfc = fluxes_at_levels.back()[idx_500];

  // UV-B should be more attenuated than visible
  double uv_ratio = flux_300_sfc / flux_300_toa;
  double vis_ratio = flux_500_sfc / flux_500_toa;
  EXPECT_LT(uv_ratio, vis_ratio);

  std::cout << "UV-B (300 nm) surface/TOA ratio: " << uv_ratio << "\n";
  std::cout << "Visible (500 nm) surface/TOA ratio: " << vis_ratio << "\n";

  EXPECT_TRUE(true);
}

/// =============================================================================
/// Bonus: Transmittance Spectrum
/// =============================================================================
/// Shows total atmospheric transmission vs wavelength

TEST(SpectralAnalysis, TransmittanceSpectrum)
{
  tuvx::ModelConfig config;
  config.wavelength_min = 280.0;
  config.wavelength_max = 700.0;
  config.n_wavelength_bins = 210;
  config.altitude_min = 0.0;
  config.altitude_max = 80.0;
  config.n_altitude_layers = 80;
  config.solar_zenith_angle = 0.0;  // Overhead sun
  config.ozone_column_DU = 300.0;

  tuvx::TuvModel model(config);
  model.UseStandardAtmosphere();
  model.AddStandardRadiators();

  auto output = model.Calculate();

  const auto& wl_grid = model.WavelengthGrid();
  const auto& alt_grid = model.AltitudeGrid();

  auto wavelengths = wl_grid.Midpoints();
  std::size_t n_wavelengths = wavelengths.size();
  std::size_t n_layers = alt_grid.Spec().n_cells;

  // Get actinic flux at TOA and surface
  auto toa_flux = output.GetActinicFlux(n_layers - 1);
  auto sfc_flux = output.GetActinicFlux(0);

  // Transmittance = surface flux / TOA flux
  std::ofstream file(kOutputDir + "transmittance_spectrum.csv");
  file << std::scientific << std::setprecision(6);
  file << "wavelength_nm,transmittance,toa_flux,surface_flux\n";

  for (std::size_t j = 0; j < n_wavelengths; ++j)
  {
    double T = (toa_flux[j] > 0) ? sfc_flux[j] / toa_flux[j] : 0.0;
    file << wavelengths[j] << "," << T << "," << toa_flux[j] << "," << sfc_flux[j] << "\n";
  }

  file.close();
  std::cout << "\nWrote: " << kOutputDir << "transmittance_spectrum.csv\n";
  std::cout << "Columns: wavelength_nm, transmittance, toa_flux, surface_flux\n";
  std::cout << "SZA: 0 deg (overhead sun), O3 column: 300 DU\n";

  EXPECT_TRUE(true);
}
