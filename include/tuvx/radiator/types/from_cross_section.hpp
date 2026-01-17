#pragma once

#include <memory>
#include <string>
#include <utility>

#include <tuvx/cross_section/cross_section.hpp>
#include <tuvx/grid/grid.hpp>
#include <tuvx/profile/profile.hpp>
#include <tuvx/radiator/radiator.hpp>

namespace tuvx
{
  /// @brief Radiator computed from absorption cross-sections
  ///
  /// FromCrossSectionRadiator computes optical properties for a molecular
  /// absorber using absorption cross-sections and number density profiles.
  /// This is the standard radiator type for gases like O3, NO2, etc.
  ///
  /// The optical depth is computed as:
  ///   τ(z, λ) = σ(T, λ) × N(z) × Δz
  ///
  /// where σ is the absorption cross-section, N is the number density,
  /// and Δz is the layer thickness.
  ///
  /// For pure absorbers, single scattering albedo = 0 and asymmetry factor = 0.
  class FromCrossSectionRadiator : public Radiator
  {
   public:
    /// @brief Construct from a cross-section
    /// @param name Radiator name (e.g., "O3")
    /// @param cross_section The absorption cross-section
    /// @param density_profile_name Name of the number density profile in warehouse
    /// @param temperature_profile_name Name of the temperature profile in warehouse
    /// @param wavelength_grid_name Name of the wavelength grid in warehouse
    /// @param altitude_grid_name Name of the altitude grid in warehouse
    FromCrossSectionRadiator(
        std::string name,
        std::unique_ptr<CrossSection> cross_section,
        std::string density_profile_name,
        std::string temperature_profile_name = "temperature",
        std::string wavelength_grid_name = "wavelength",
        std::string altitude_grid_name = "altitude")
        : cross_section_(std::move(cross_section)),
          density_profile_name_(std::move(density_profile_name)),
          temperature_profile_name_(std::move(temperature_profile_name)),
          wavelength_grid_name_(std::move(wavelength_grid_name)),
          altitude_grid_name_(std::move(altitude_grid_name))
    {
      name_ = std::move(name);
    }

    /// @brief Clone this radiator
    std::unique_ptr<Radiator> Clone() const override
    {
      return std::make_unique<FromCrossSectionRadiator>(
          name_,
          cross_section_->Clone(),
          density_profile_name_,
          temperature_profile_name_,
          wavelength_grid_name_,
          altitude_grid_name_);
    }

    /// @brief Update optical state from current atmospheric conditions
    ///
    /// Computes optical depth at each layer and wavelength using:
    ///   τ[i][j] = σ(T[i], λ[j]) × N[i] × Δz[i]
    void UpdateState(const GridWarehouse& grids, const ProfileWarehouse& profiles) override
    {
      // Get grids
      const auto& wl_grid = grids.Get(wavelength_grid_name_, "nm");
      const auto& alt_grid = grids.Get(altitude_grid_name_, "km");

      // Get profiles
      const auto& density_profile = profiles.Get(density_profile_name_, "molecules/cm^3");
      const auto& temperature_profile = profiles.Get(temperature_profile_name_, "K");

      std::size_t n_layers = alt_grid.Spec().n_cells;
      std::size_t n_wavelengths = wl_grid.Spec().n_cells;

      // Initialize state
      state_.Initialize(n_layers, n_wavelengths);

      // Get layer thicknesses (convert km to cm for consistency with cross-sections)
      auto deltas = alt_grid.Deltas();

      // Get number densities at each layer
      auto densities = density_profile.MidValues();

      // Calculate cross-section profile [n_layers][n_wavelengths]
      auto xs_profile = cross_section_->CalculateProfile(wl_grid, alt_grid, temperature_profile);

      // Compute optical depths
      for (std::size_t i = 0; i < n_layers; ++i)
      {
        // Convert layer thickness from km to cm
        double delta_z_cm = std::abs(deltas[i]) * 1.0e5;  // km -> cm

        for (std::size_t j = 0; j < n_wavelengths; ++j)
        {
          // τ = σ × N × Δz
          state_.optical_depth[i][j] = xs_profile[i][j] * densities[i] * delta_z_cm;
        }

        // Pure absorber: ω = 0, g = 0 (already initialized to zero)
      }
    }

    /// @brief Get the cross-section used by this radiator
    const CrossSection& GetCrossSection() const
    {
      return *cross_section_;
    }

    /// @brief Get the density profile name
    const std::string& DensityProfileName() const
    {
      return density_profile_name_;
    }

   private:
    std::unique_ptr<CrossSection> cross_section_;
    std::string density_profile_name_;
    std::string temperature_profile_name_;
    std::string wavelength_grid_name_;
    std::string altitude_grid_name_;
  };

}  // namespace tuvx
