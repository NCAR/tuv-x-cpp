#pragma once

#include <memory>
#include <string>
#include <vector>

#include <tuvx/grid/grid.hpp>
#include <tuvx/profile/profile.hpp>

namespace tuvx
{
  /// @brief Base class for wavelength-dependent absorption cross-sections
  ///
  /// CrossSection provides the interface for calculating molecular absorption
  /// cross-sections as a function of wavelength and temperature. These are
  /// fundamental inputs for photolysis rate calculations.
  ///
  /// Cross-sections have units of cm^2/molecule and represent the effective
  /// area for photon absorption at each wavelength.
  ///
  /// Derived classes implement specific cross-section parameterizations for
  /// different molecular species (O3, NO2, etc.).
  class CrossSection
  {
   public:
    virtual ~CrossSection() = default;

    /// @brief Clone this cross-section (for polymorphic copy)
    /// @return A unique_ptr to a copy of this cross-section
    virtual std::unique_ptr<CrossSection> Clone() const = 0;

    /// @brief Calculate cross-section values on wavelength grid
    /// @param wavelength_grid Wavelength grid (midpoints used) [nm]
    /// @param temperature Temperature [K]
    /// @return Cross-section values at each wavelength bin [cm^2/molecule]
    ///
    /// Returns values at wavelength grid midpoints. The returned vector
    /// has size equal to wavelength_grid.Spec().n_cells.
    virtual std::vector<double> Calculate(const Grid& wavelength_grid, double temperature) const = 0;

    /// @brief Calculate cross-section profile (altitude-dependent)
    /// @param wavelength_grid Wavelength grid
    /// @param altitude_grid Altitude grid
    /// @param temperature_profile Temperature vs altitude [K]
    /// @return 2D array [n_altitude_cells][n_wavelength_cells] of cross-sections
    ///
    /// Calculates cross-sections at each altitude level using the local
    /// temperature from the temperature profile. The default implementation
    /// calls Calculate() for each altitude.
    virtual std::vector<std::vector<double>> CalculateProfile(
        const Grid& wavelength_grid,
        const Grid& altitude_grid,
        const Profile& temperature_profile) const
    {
      std::size_t n_layers = altitude_grid.Spec().n_cells;
      std::size_t n_wavelengths = wavelength_grid.Spec().n_cells;

      std::vector<std::vector<double>> result(n_layers);

      auto temperatures = temperature_profile.MidValues();

      for (std::size_t i = 0; i < n_layers; ++i)
      {
        result[i] = Calculate(wavelength_grid, temperatures[i]);
      }

      return result;
    }

    /// @brief Get the name of this cross-section
    /// @return The species or reaction name (e.g., "O3", "NO2")
    const std::string& Name() const
    {
      return name_;
    }

    /// @brief Get the units of this cross-section
    /// @return The units string (typically "cm^2")
    const std::string& Units() const
    {
      return units_;
    }

   protected:
    std::string name_{};
    std::string units_{ "cm^2" };
  };

}  // namespace tuvx
