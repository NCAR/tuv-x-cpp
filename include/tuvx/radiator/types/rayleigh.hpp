#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <utility>

#include <tuvx/grid/grid.hpp>
#include <tuvx/profile/profile.hpp>
#include <tuvx/radiator/radiator.hpp>
#include <tuvx/util/constants.hpp>

namespace tuvx
{
  /// @brief Rayleigh (molecular) scattering radiator
  ///
  /// Computes Rayleigh scattering optical properties for the atmosphere.
  /// Rayleigh scattering is pure scattering (ω = 1) with isotropic phase
  /// function (g = 0) and cross-section proportional to λ^-4.
  ///
  /// The Rayleigh scattering cross-section is:
  ///   σ_R(λ) = (24π³/N²λ⁴) × ((n²-1)/(n²+2))² × F(λ)
  ///
  /// where n is the refractive index of air, N is Loschmidt's number,
  /// and F(λ) is the King correction factor (~1.05 for air).
  ///
  /// For simplicity, we use the parameterization:
  ///   σ_R(λ) = A × (λ/λ₀)^(-4-B)
  ///
  /// where A = 4.02e-28 cm², λ₀ = 1 nm, and B ≈ 0.04 accounts for
  /// the wavelength dependence of the refractive index.
  ///
  /// Reference:
  /// - Bodhaine et al. (1999): On Rayleigh Optical Depth Calculations,
  ///   J. Atmos. Ocean. Tech., 16, 1854-1861
  class RayleighRadiator : public Radiator
  {
   public:
    /// @brief Construct Rayleigh radiator
    /// @param air_density_profile_name Name of the air density profile in warehouse
    /// @param wavelength_grid_name Name of the wavelength grid in warehouse
    /// @param altitude_grid_name Name of the altitude grid in warehouse
    RayleighRadiator(
        std::string air_density_profile_name = "air_density",
        std::string wavelength_grid_name = "wavelength",
        std::string altitude_grid_name = "altitude")
        : air_density_profile_name_(std::move(air_density_profile_name)),
          wavelength_grid_name_(std::move(wavelength_grid_name)),
          altitude_grid_name_(std::move(altitude_grid_name))
    {
      name_ = "rayleigh";
    }

    /// @brief Clone this radiator
    std::unique_ptr<Radiator> Clone() const override
    {
      return std::make_unique<RayleighRadiator>(
          air_density_profile_name_, wavelength_grid_name_, altitude_grid_name_);
    }

    /// @brief Update optical state from current atmospheric conditions
    ///
    /// Computes Rayleigh scattering optical depth at each layer and wavelength:
    ///   τ_R[i][j] = σ_R(λ[j]) × N_air[i] × Δz[i]
    ///
    /// Sets ω = 1 and g = 0 for all layers and wavelengths.
    void UpdateState(const GridWarehouse& grids, const ProfileWarehouse& profiles) override
    {
      // Get grids
      const auto& wl_grid = grids.Get(wavelength_grid_name_, "nm");
      const auto& alt_grid = grids.Get(altitude_grid_name_, "km");

      // Get air density profile
      const auto& air_density_profile = profiles.Get(air_density_profile_name_, "molecules/cm^3");

      std::size_t n_layers = alt_grid.Spec().n_cells;
      std::size_t n_wavelengths = wl_grid.Spec().n_cells;

      // Initialize state
      state_.Initialize(n_layers, n_wavelengths);

      // Get layer thicknesses and wavelength midpoints
      auto deltas = alt_grid.Deltas();
      auto wavelengths = wl_grid.Midpoints();
      auto densities = air_density_profile.MidValues();

      // Compute optical depths
      for (std::size_t i = 0; i < n_layers; ++i)
      {
        // Convert layer thickness from km to cm
        double delta_z_cm = std::abs(deltas[i]) * 1.0e5;  // km -> cm

        for (std::size_t j = 0; j < n_wavelengths; ++j)
        {
          // Rayleigh cross-section
          double sigma = RayleighCrossSection(wavelengths[j]);

          // τ = σ × N × Δz
          state_.optical_depth[i][j] = sigma * densities[i] * delta_z_cm;

          // Pure scattering: ω = 1
          state_.single_scattering_albedo[i][j] = 1.0;

          // Isotropic scattering: g = 0
          state_.asymmetry_factor[i][j] = 0.0;
        }
      }
    }

    /// @brief Calculate Rayleigh scattering cross-section
    /// @param wavelength_nm Wavelength in nanometers
    /// @return Cross-section in cm²/molecule
    ///
    /// Uses the parameterization from Bodhaine et al. (1999):
    ///   σ_R(λ) = σ_ref × (λ_ref/λ)^(4+ε)
    ///
    /// where σ_ref = 4.02e-28 cm² at λ_ref = 1000 nm (1 μm)
    /// and ε ≈ 0.04 accounts for dispersion of the refractive index.
    ///
    /// This gives σ ≈ 1.7e-26 cm² at 400 nm, consistent with
    /// literature values for standard air.
    static double RayleighCrossSection(double wavelength_nm)
    {
      // Rayleigh cross-section at reference wavelength 1000 nm
      constexpr double sigma_ref = 4.02e-28;  // cm² at 1000 nm
      constexpr double lambda_ref = 1000.0;   // nm
      constexpr double exponent = 4.04;       // 4 + dispersion correction

      double ratio = lambda_ref / wavelength_nm;
      return sigma_ref * std::pow(ratio, exponent);
    }

   private:
    std::string air_density_profile_name_;
    std::string wavelength_grid_name_;
    std::string altitude_grid_name_;
  };

}  // namespace tuvx
