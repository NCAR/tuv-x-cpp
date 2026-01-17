#pragma once

#include <cstddef>
#include <vector>

namespace tuvx
{
  /// @brief Computed radiation quantities at all altitudes and wavelengths
  ///
  /// RadiationField holds the results of a radiative transfer calculation.
  /// It contains the direct and diffuse components of irradiance and actinic
  /// flux at each level of the atmosphere.
  ///
  /// The field is organized by level (layer boundaries) and wavelength:
  /// - n_levels = n_layers + 1 (includes surface and TOA)
  /// - Levels are ordered from surface (index 0) to top of atmosphere
  ///
  /// Key quantities:
  /// - **Irradiance** (E): Power per unit area [W/m^2 or photons/cm^2/s]
  /// - **Actinic flux** (F): Photon flux from all directions [photons/cm^2/s]
  ///
  /// Actinic flux is relevant for photochemistry because molecules can be
  /// photolyzed by photons from any direction.
  struct RadiationField
  {
    /// @brief Direct (unscattered) spectral irradiance [n_levels][n_wavelengths]
    ///
    /// Solar radiation that has not been scattered or absorbed.
    /// Zero below the level where direct beam is fully attenuated.
    std::vector<std::vector<double>> direct_irradiance;

    /// @brief Diffuse upwelling irradiance [n_levels][n_wavelengths]
    ///
    /// Scattered radiation traveling upward (from surface reflections
    /// and backscattering).
    std::vector<std::vector<double>> diffuse_up;

    /// @brief Diffuse downwelling irradiance [n_levels][n_wavelengths]
    ///
    /// Scattered radiation traveling downward (from forward scattering
    /// and multiple scattering).
    std::vector<std::vector<double>> diffuse_down;

    /// @brief Direct actinic flux [n_levels][n_wavelengths]
    ///
    /// Actinic flux from the direct solar beam. Related to direct
    /// irradiance by: F_direct = E_direct / μ₀ where μ₀ = cos(SZA).
    std::vector<std::vector<double>> actinic_flux_direct;

    /// @brief Diffuse actinic flux [n_levels][n_wavelengths]
    ///
    /// Actinic flux from scattered radiation (both up and down).
    std::vector<std::vector<double>> actinic_flux_diffuse;

    /// @brief Initialize field with zeros for given dimensions
    /// @param n_levels Number of altitude levels (n_layers + 1)
    /// @param n_wavelengths Number of wavelength bins
    void Initialize(std::size_t n_levels, std::size_t n_wavelengths)
    {
      direct_irradiance.assign(n_levels, std::vector<double>(n_wavelengths, 0.0));
      diffuse_up.assign(n_levels, std::vector<double>(n_wavelengths, 0.0));
      diffuse_down.assign(n_levels, std::vector<double>(n_wavelengths, 0.0));
      actinic_flux_direct.assign(n_levels, std::vector<double>(n_wavelengths, 0.0));
      actinic_flux_diffuse.assign(n_levels, std::vector<double>(n_wavelengths, 0.0));
    }

    /// @brief Get number of levels
    std::size_t NumberOfLevels() const
    {
      return direct_irradiance.size();
    }

    /// @brief Get number of wavelengths
    std::size_t NumberOfWavelengths() const
    {
      if (direct_irradiance.empty())
      {
        return 0;
      }
      return direct_irradiance[0].size();
    }

    /// @brief Check if field is empty
    bool Empty() const
    {
      return direct_irradiance.empty();
    }

    /// @brief Get total actinic flux at a level (direct + diffuse)
    /// @param level_index The level index (0 = surface)
    /// @return Vector of total actinic flux at each wavelength
    std::vector<double> TotalActinicFlux(std::size_t level_index) const
    {
      if (level_index >= NumberOfLevels())
      {
        return {};
      }

      std::size_t n_wavelengths = NumberOfWavelengths();
      std::vector<double> total(n_wavelengths);

      for (std::size_t j = 0; j < n_wavelengths; ++j)
      {
        total[j] = actinic_flux_direct[level_index][j] + actinic_flux_diffuse[level_index][j];
      }

      return total;
    }

    /// @brief Get total downwelling irradiance at a level (direct + diffuse)
    /// @param level_index The level index
    /// @return Vector of total downwelling irradiance at each wavelength
    std::vector<double> TotalDownwelling(std::size_t level_index) const
    {
      if (level_index >= NumberOfLevels())
      {
        return {};
      }

      std::size_t n_wavelengths = NumberOfWavelengths();
      std::vector<double> total(n_wavelengths);

      for (std::size_t j = 0; j < n_wavelengths; ++j)
      {
        total[j] = direct_irradiance[level_index][j] + diffuse_down[level_index][j];
      }

      return total;
    }

    /// @brief Scale all radiation quantities by a factor
    /// @param factor Scale factor (e.g., for time averaging)
    void Scale(double factor)
    {
      for (std::size_t i = 0; i < NumberOfLevels(); ++i)
      {
        for (std::size_t j = 0; j < NumberOfWavelengths(); ++j)
        {
          direct_irradiance[i][j] *= factor;
          diffuse_up[i][j] *= factor;
          diffuse_down[i][j] *= factor;
          actinic_flux_direct[i][j] *= factor;
          actinic_flux_diffuse[i][j] *= factor;
        }
      }
    }

    /// @brief Accumulate another radiation field into this one
    /// @param other The field to add
    ///
    /// Useful for integrating over time or solar zenith angle.
    void Accumulate(const RadiationField& other)
    {
      if (other.Empty())
      {
        return;
      }

      if (Empty())
      {
        *this = other;
        return;
      }

      std::size_t n_levels = NumberOfLevels();
      std::size_t n_wavelengths = NumberOfWavelengths();

      for (std::size_t i = 0; i < n_levels; ++i)
      {
        for (std::size_t j = 0; j < n_wavelengths; ++j)
        {
          direct_irradiance[i][j] += other.direct_irradiance[i][j];
          diffuse_up[i][j] += other.diffuse_up[i][j];
          diffuse_down[i][j] += other.diffuse_down[i][j];
          actinic_flux_direct[i][j] += other.actinic_flux_direct[i][j];
          actinic_flux_diffuse[i][j] += other.actinic_flux_diffuse[i][j];
        }
      }
    }

    /// @brief Get surface-level total actinic flux
    /// @return Total actinic flux at surface (level 0)
    std::vector<double> SurfaceActinicFlux() const
    {
      return TotalActinicFlux(0);
    }

    /// @brief Get surface-level global irradiance (direct + diffuse down)
    /// @return Global irradiance at surface
    std::vector<double> SurfaceGlobalIrradiance() const
    {
      return TotalDownwelling(0);
    }
  };

}  // namespace tuvx
