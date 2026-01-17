#pragma once

#include <memory>
#include <string>
#include <vector>

#include <tuvx/grid/grid.hpp>
#include <tuvx/profile/profile.hpp>

namespace tuvx
{
  /// @brief Base class for photochemical quantum yields
  ///
  /// QuantumYield provides the interface for calculating the probability
  /// that a photon absorbed by a molecule leads to a specific photochemical
  /// product. Quantum yields are dimensionless values between 0 and 1.
  ///
  /// For a photolysis reaction: AB + hv -> A + B
  /// The quantum yield phi gives the fraction of absorbed photons that
  /// result in dissociation.
  ///
  /// Quantum yields may depend on:
  /// - Wavelength of the absorbed photon
  /// - Temperature (internal energy distribution)
  /// - Air density (collisional quenching)
  ///
  /// The photolysis rate constant J is computed as:
  /// J = integral[ F(lambda) * sigma(lambda) * phi(lambda) ] d_lambda
  /// where F is actinic flux, sigma is cross-section, phi is quantum yield.
  class QuantumYield
  {
   public:
    virtual ~QuantumYield() = default;

    /// @brief Clone this quantum yield (for polymorphic copy)
    /// @return A unique_ptr to a copy of this quantum yield
    virtual std::unique_ptr<QuantumYield> Clone() const = 0;

    /// @brief Calculate quantum yield values on wavelength grid
    /// @param wavelength_grid Wavelength grid (midpoints used) [nm]
    /// @param temperature Temperature [K]
    /// @param air_density Air number density [molecules/cm^3] (for quenching)
    /// @return Quantum yield values at each wavelength bin [0-1]
    ///
    /// The air_density parameter is used for pressure-dependent quenching
    /// in some quantum yields. Pass 0.0 if not applicable.
    virtual std::vector<double>
    Calculate(const Grid& wavelength_grid, double temperature, double air_density = 0.0) const = 0;

    /// @brief Calculate quantum yield profile (altitude-dependent)
    /// @param wavelength_grid Wavelength grid
    /// @param altitude_grid Altitude grid
    /// @param temperature_profile Temperature vs altitude [K]
    /// @param air_density_profile Air density vs altitude [molecules/cm^3]
    /// @return 2D array [n_altitude_cells][n_wavelength_cells] of quantum yields
    ///
    /// Calculates quantum yields at each altitude level using the local
    /// temperature and air density from the profiles.
    virtual std::vector<std::vector<double>> CalculateProfile(
        const Grid& wavelength_grid,
        const Grid& altitude_grid,
        const Profile& temperature_profile,
        const Profile& air_density_profile) const
    {
      std::size_t n_layers = altitude_grid.Spec().n_cells;

      std::vector<std::vector<double>> result(n_layers);

      auto temperatures = temperature_profile.MidValues();
      auto air_densities = air_density_profile.MidValues();

      for (std::size_t i = 0; i < n_layers; ++i)
      {
        result[i] = Calculate(wavelength_grid, temperatures[i], air_densities[i]);
      }

      return result;
    }

    /// @brief Get the name of this quantum yield
    /// @return The reaction name (e.g., "O3->O(1D)+O2")
    const std::string& Name() const
    {
      return name_;
    }

    /// @brief Get the reactant species name
    /// @return The parent molecule (e.g., "O3")
    const std::string& Reactant() const
    {
      return reactant_;
    }

    /// @brief Get the product description
    /// @return The products (e.g., "O(1D)+O2")
    const std::string& Products() const
    {
      return products_;
    }

   protected:
    std::string name_{};
    std::string reactant_{};
    std::string products_{};
  };

}  // namespace tuvx
