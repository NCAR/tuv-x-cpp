#pragma once

#include <string>
#include <vector>

#include <tuvx/cross_section/cross_section.hpp>
#include <tuvx/grid/grid.hpp>
#include <tuvx/quantum_yield/quantum_yield.hpp>
#include <tuvx/radiation_field/radiation_field.hpp>

namespace tuvx
{
  /// @brief Photolysis rate (J-value) calculator
  ///
  /// Computes photolysis rates by integrating the product of:
  /// - Actinic flux F(λ) [photons/cm^2/s]
  /// - Absorption cross-section σ(λ) [cm^2]
  /// - Quantum yield φ(λ) [dimensionless]
  ///
  /// J = ∫ F(λ) × σ(λ) × φ(λ) dλ  [s^-1]
  ///
  /// The calculation is performed at each altitude level.
  class PhotolysisRateCalculator
  {
   public:
    /// @brief Result of photolysis rate calculation
    struct Result
    {
      /// Reaction name
      std::string reaction_name;

      /// Photolysis rates at each level [s^-1]
      std::vector<double> rates;

      /// Number of levels
      std::size_t NumberOfLevels() const
      {
        return rates.size();
      }

      /// Get rate at surface (level 0)
      double SurfaceRate() const
      {
        return rates.empty() ? 0.0 : rates[0];
      }
    };

    /// @brief Construct calculator for a specific reaction
    /// @param reaction_name Name of the photolysis reaction
    /// @param cross_section Absorption cross-section
    /// @param quantum_yield Quantum yield for products
    PhotolysisRateCalculator(
        std::string reaction_name,
        const CrossSection* cross_section,
        const QuantumYield* quantum_yield)
        : reaction_name_(std::move(reaction_name)),
          cross_section_(cross_section),
          quantum_yield_(quantum_yield)
    {
    }

    /// @brief Get reaction name
    const std::string& ReactionName() const
    {
      return reaction_name_;
    }

    /// @brief Calculate photolysis rates
    /// @param radiation_field Computed actinic flux at all levels
    /// @param wavelength_grid Wavelength grid
    /// @param temperature_profile Temperature at each layer (for T-dependent xs/qy)
    /// @return Photolysis rates at each level
    Result Calculate(
        const RadiationField& radiation_field,
        const Grid& wavelength_grid,
        const std::vector<double>& temperature_profile = {}) const
    {
      Result result;
      result.reaction_name = reaction_name_;

      if (radiation_field.Empty() || !cross_section_ || !quantum_yield_)
      {
        return result;
      }

      std::size_t n_levels = radiation_field.NumberOfLevels();
      std::size_t n_wavelengths = radiation_field.NumberOfWavelengths();

      result.rates.resize(n_levels, 0.0);

      // Get wavelength bin widths for integration
      auto deltas = wavelength_grid.Deltas();

      // Calculate at each level
      for (std::size_t level = 0; level < n_levels; ++level)
      {
        // Get temperature for this level (use layer below or default)
        double temperature = 298.0;  // Default
        if (!temperature_profile.empty())
        {
          std::size_t layer = (level > 0) ? level - 1 : 0;
          if (layer < temperature_profile.size())
          {
            temperature = temperature_profile[layer];
          }
        }

        // Get cross-section and quantum yield at this temperature
        auto xs_values = cross_section_->Calculate(wavelength_grid, temperature);
        auto qy_values = quantum_yield_->Calculate(wavelength_grid, temperature);

        // Get total actinic flux at this level
        auto actinic_flux = radiation_field.TotalActinicFlux(level);

        // Integrate: J = ∫ F(λ) × σ(λ) × φ(λ) dλ
        double j_value = 0.0;
        for (std::size_t j = 0; j < n_wavelengths; ++j)
        {
          j_value += actinic_flux[j] * xs_values[j] * qy_values[j] * std::abs(deltas[j]);
        }

        result.rates[level] = j_value;
      }

      return result;
    }

    /// @brief Calculate photolysis rate at a single level
    /// @param actinic_flux Total actinic flux at wavelengths [photons/cm^2/s/nm]
    /// @param wavelength_grid Wavelength grid
    /// @param temperature Temperature [K]
    /// @return Photolysis rate [s^-1]
    double CalculateAtLevel(
        const std::vector<double>& actinic_flux,
        const Grid& wavelength_grid,
        double temperature = 298.0) const
    {
      if (actinic_flux.empty() || !cross_section_ || !quantum_yield_)
      {
        return 0.0;
      }

      auto xs_values = cross_section_->Calculate(wavelength_grid, temperature);
      auto qy_values = quantum_yield_->Calculate(wavelength_grid, temperature);
      auto deltas = wavelength_grid.Deltas();

      double j_value = 0.0;
      std::size_t n = std::min({ actinic_flux.size(), xs_values.size(), qy_values.size() });

      for (std::size_t j = 0; j < n; ++j)
      {
        j_value += actinic_flux[j] * xs_values[j] * qy_values[j] * std::abs(deltas[j]);
      }

      return j_value;
    }

   private:
    std::string reaction_name_;
    const CrossSection* cross_section_;
    const QuantumYield* quantum_yield_;
  };

  /// @brief Collection of photolysis rate calculators
  class PhotolysisRateSet
  {
   public:
    /// @brief Add a reaction to the set
    void AddReaction(
        const std::string& name,
        const CrossSection* cross_section,
        const QuantumYield* quantum_yield)
    {
      calculators_.emplace_back(name, cross_section, quantum_yield);
    }

    /// @brief Calculate all photolysis rates
    /// @param radiation_field Computed actinic flux
    /// @param wavelength_grid Wavelength grid
    /// @param temperature_profile Temperature profile
    /// @return Vector of results for each reaction
    std::vector<PhotolysisRateCalculator::Result> CalculateAll(
        const RadiationField& radiation_field,
        const Grid& wavelength_grid,
        const std::vector<double>& temperature_profile = {}) const
    {
      std::vector<PhotolysisRateCalculator::Result> results;
      results.reserve(calculators_.size());

      for (const auto& calc : calculators_)
      {
        results.push_back(calc.Calculate(radiation_field, wavelength_grid, temperature_profile));
      }

      return results;
    }

    /// @brief Get number of reactions
    std::size_t Size() const
    {
      return calculators_.size();
    }

    /// @brief Check if empty
    bool Empty() const
    {
      return calculators_.empty();
    }

    /// @brief Get reaction names
    std::vector<std::string> ReactionNames() const
    {
      std::vector<std::string> names;
      names.reserve(calculators_.size());
      for (const auto& calc : calculators_)
      {
        names.push_back(calc.ReactionName());
      }
      return names;
    }

   private:
    std::vector<PhotolysisRateCalculator> calculators_;
  };

}  // namespace tuvx
