#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <tuvx/grid/grid.hpp>
#include <tuvx/photolysis/photolysis_rate.hpp>
#include <tuvx/radiation_field/radiation_field.hpp>

namespace tuvx
{
  /// @brief Output from TUV model calculation
  ///
  /// Contains all computed radiation fields and photolysis rates,
  /// along with metadata about the calculation.
  class ModelOutput
  {
   public:
    // ========================================================================
    // Calculation Metadata
    // ========================================================================

    /// Solar zenith angle used for calculation [degrees]
    double solar_zenith_angle{ 0.0 };

    /// Day of year used for calculation
    int day_of_year{ 1 };

    /// Earth-Sun distance used [AU]
    double earth_sun_distance{ 1.0 };

    /// Was this a daytime calculation?
    bool is_daytime{ true };

    /// Was spherical geometry used?
    bool used_spherical_geometry{ true };

    // ========================================================================
    // Grids
    // ========================================================================

    /// Wavelength grid used for calculation
    Grid wavelength_grid;

    /// Altitude grid used for calculation
    Grid altitude_grid;

    // ========================================================================
    // Radiation Field
    // ========================================================================

    /// Computed radiation field at all levels and wavelengths
    RadiationField radiation_field;

    // ========================================================================
    // Photolysis Rates
    // ========================================================================

    /// Photolysis rate results for all configured reactions
    std::vector<PhotolysisRateCalculator::Result> photolysis_rates;

    // ========================================================================
    // Accessors
    // ========================================================================

    /// @brief Check if output is empty (no calculation performed)
    bool Empty() const
    {
      return radiation_field.Empty();
    }

    /// @brief Get number of altitude levels
    std::size_t NumberOfLevels() const
    {
      return radiation_field.NumberOfLevels();
    }

    /// @brief Get number of wavelength bins
    std::size_t NumberOfWavelengths() const
    {
      return radiation_field.NumberOfWavelengths();
    }

    /// @brief Get number of photolysis reactions
    std::size_t NumberOfReactions() const
    {
      return photolysis_rates.size();
    }

    /// @brief Get list of photolysis reaction names
    std::vector<std::string> ReactionNames() const
    {
      std::vector<std::string> names;
      names.reserve(photolysis_rates.size());
      for (const auto& result : photolysis_rates)
      {
        names.push_back(result.reaction_name);
      }
      return names;
    }

    /// @brief Get photolysis rate for a specific reaction at a specific level
    /// @param reaction_name Name of the photolysis reaction
    /// @param level Altitude level index (0 = surface)
    /// @return Photolysis rate [s⁻¹]
    /// @throws std::out_of_range if reaction not found or level out of bounds
    double GetPhotolysisRate(const std::string& reaction_name, std::size_t level) const
    {
      for (const auto& result : photolysis_rates)
      {
        if (result.reaction_name == reaction_name)
        {
          if (level >= result.rates.size())
          {
            throw std::out_of_range("Level index out of bounds");
          }
          return result.rates[level];
        }
      }
      throw std::out_of_range("Reaction not found: " + reaction_name);
    }

    /// @brief Get photolysis rate profile for a specific reaction
    /// @param reaction_name Name of the photolysis reaction
    /// @return Vector of photolysis rates at all levels [s⁻¹]
    /// @throws std::out_of_range if reaction not found
    const std::vector<double>& GetPhotolysisRateProfile(const std::string& reaction_name) const
    {
      for (const auto& result : photolysis_rates)
      {
        if (result.reaction_name == reaction_name)
        {
          return result.rates;
        }
      }
      throw std::out_of_range("Reaction not found: " + reaction_name);
    }

    /// @brief Get surface photolysis rate for a specific reaction
    /// @param reaction_name Name of the photolysis reaction
    /// @return Photolysis rate at surface [s⁻¹]
    double GetSurfacePhotolysisRate(const std::string& reaction_name) const
    {
      return GetPhotolysisRate(reaction_name, 0);
    }

    /// @brief Get total actinic flux at a specific level
    /// @param level Altitude level index (0 = surface)
    /// @return Vector of total actinic flux at each wavelength [photons/cm²/s]
    std::vector<double> GetActinicFlux(std::size_t level) const
    {
      return radiation_field.TotalActinicFlux(level);
    }

    /// @brief Get direct actinic flux at a specific level
    /// @param level Altitude level index (0 = surface)
    /// @return Vector of direct actinic flux at each wavelength [photons/cm²/s]
    std::vector<double> GetDirectActinicFlux(std::size_t level) const
    {
      if (level >= NumberOfLevels())
      {
        return {};
      }
      return radiation_field.actinic_flux_direct[level];
    }

    /// @brief Get diffuse actinic flux at a specific level
    /// @param level Altitude level index (0 = surface)
    /// @return Vector of diffuse actinic flux at each wavelength [photons/cm²/s]
    std::vector<double> GetDiffuseActinicFlux(std::size_t level) const
    {
      if (level >= NumberOfLevels())
      {
        return {};
      }
      return radiation_field.actinic_flux_diffuse[level];
    }

    /// @brief Get direct irradiance at a specific level
    /// @param level Altitude level index (0 = surface)
    /// @return Vector of direct irradiance at each wavelength [W/m² or photons/cm²/s]
    std::vector<double> GetDirectIrradiance(std::size_t level) const
    {
      if (level >= NumberOfLevels())
      {
        return {};
      }
      return radiation_field.direct_irradiance[level];
    }

    /// @brief Get downwelling diffuse irradiance at a specific level
    /// @param level Altitude level index (0 = surface)
    /// @return Vector of downwelling diffuse at each wavelength
    std::vector<double> GetDiffuseDownIrradiance(std::size_t level) const
    {
      if (level >= NumberOfLevels())
      {
        return {};
      }
      return radiation_field.diffuse_down[level];
    }

    /// @brief Get upwelling diffuse irradiance at a specific level
    /// @param level Altitude level index (0 = surface)
    /// @return Vector of upwelling diffuse at each wavelength
    std::vector<double> GetDiffuseUpIrradiance(std::size_t level) const
    {
      if (level >= NumberOfLevels())
      {
        return {};
      }
      return radiation_field.diffuse_up[level];
    }

    // ========================================================================
    // Integrated Quantities
    // ========================================================================

    /// @brief Get spectrally integrated actinic flux at a level
    /// @param level Altitude level index
    /// @return Total actinic flux integrated over all wavelengths
    double GetIntegratedActinicFlux(std::size_t level) const
    {
      auto flux = GetActinicFlux(level);
      auto deltas = wavelength_grid.Deltas();

      double total = 0.0;
      std::size_t n = std::min(flux.size(), deltas.size());
      for (std::size_t i = 0; i < n; ++i)
      {
        total += flux[i] * std::abs(deltas[i]);
      }
      return total;
    }

    /// @brief Get UV-B actinic flux (280-315 nm) at a level
    /// @param level Altitude level index
    /// @return UV-B actinic flux integrated over 280-315 nm
    double GetUVBActinicFlux(std::size_t level) const
    {
      return GetBandActinicFlux(level, 280.0, 315.0);
    }

    /// @brief Get UV-A actinic flux (315-400 nm) at a level
    /// @param level Altitude level index
    /// @return UV-A actinic flux integrated over 315-400 nm
    double GetUVAActinicFlux(std::size_t level) const
    {
      return GetBandActinicFlux(level, 315.0, 400.0);
    }

    /// @brief Get actinic flux integrated over a wavelength band
    /// @param level Altitude level index
    /// @param wl_min Minimum wavelength [nm]
    /// @param wl_max Maximum wavelength [nm]
    /// @return Actinic flux integrated over the band
    double GetBandActinicFlux(std::size_t level, double wl_min, double wl_max) const
    {
      auto flux = GetActinicFlux(level);
      auto midpoints = wavelength_grid.Midpoints();
      auto deltas = wavelength_grid.Deltas();

      double total = 0.0;
      std::size_t n = std::min({ flux.size(), midpoints.size(), deltas.size() });
      for (std::size_t i = 0; i < n; ++i)
      {
        if (midpoints[i] >= wl_min && midpoints[i] <= wl_max)
        {
          total += flux[i] * std::abs(deltas[i]);
        }
      }
      return total;
    }

    // ========================================================================
    // Summary Statistics
    // ========================================================================

    /// @brief Get maximum photolysis rate for a reaction (typically at TOA)
    double GetMaxPhotolysisRate(const std::string& reaction_name) const
    {
      const auto& rates = GetPhotolysisRateProfile(reaction_name);
      if (rates.empty())
        return 0.0;
      return *std::max_element(rates.begin(), rates.end());
    }

    /// @brief Print summary of output to string
    std::string Summary() const
    {
      std::string s;
      s += "TUV Model Output Summary\n";
      s += "========================\n";
      s += "SZA: " + std::to_string(solar_zenith_angle) + " degrees\n";
      s += "Daytime: " + std::string(is_daytime ? "yes" : "no") + "\n";
      s += "Levels: " + std::to_string(NumberOfLevels()) + "\n";
      s += "Wavelengths: " + std::to_string(NumberOfWavelengths()) + "\n";
      s += "Reactions: " + std::to_string(NumberOfReactions()) + "\n";

      if (!photolysis_rates.empty())
      {
        s += "\nSurface J-values:\n";
        for (const auto& result : photolysis_rates)
        {
          if (!result.rates.empty())
          {
            s += "  " + result.reaction_name + ": " + std::to_string(result.rates[0]) + " s^-1\n";
          }
        }
      }

      return s;
    }
  };

}  // namespace tuvx
