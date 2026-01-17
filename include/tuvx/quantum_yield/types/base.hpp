#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <tuvx/interpolation/linear_interpolator.hpp>
#include <tuvx/quantum_yield/quantum_yield.hpp>

namespace tuvx
{
  /// @brief Simple constant quantum yield
  ///
  /// ConstantQuantumYield returns the same value for all wavelengths,
  /// temperatures, and air densities. This is appropriate for reactions
  /// where the quantum yield is wavelength-independent.
  class ConstantQuantumYield : public QuantumYield
  {
   public:
    /// @brief Construct a constant quantum yield
    /// @param name Reaction name (e.g., "NO2->NO+O")
    /// @param reactant Parent molecule
    /// @param products Product description
    /// @param value The constant quantum yield value [0-1]
    ConstantQuantumYield(std::string name, std::string reactant, std::string products, double value)
        : value_(value)
    {
      name_ = std::move(name);
      reactant_ = std::move(reactant);
      products_ = std::move(products);
    }

    /// @brief Clone this quantum yield
    std::unique_ptr<QuantumYield> Clone() const override
    {
      return std::make_unique<ConstantQuantumYield>(name_, reactant_, products_, value_);
    }

    /// @brief Calculate quantum yield (returns constant value)
    std::vector<double>
    Calculate(const Grid& wavelength_grid, double /*temperature*/, double /*air_density*/ = 0.0) const override
    {
      std::size_t n_wavelengths = wavelength_grid.Spec().n_cells;
      return std::vector<double>(n_wavelengths, value_);
    }

    /// @brief Get the constant value
    double Value() const
    {
      return value_;
    }

   private:
    double value_;
  };

  /// @brief Wavelength-dependent quantum yield from lookup table
  ///
  /// BaseQuantumYield provides a simple lookup table implementation where
  /// the quantum yield varies with wavelength but not with temperature
  /// or air density.
  class BaseQuantumYield : public QuantumYield
  {
   public:
    /// @brief Construct from wavelength-dependent data
    /// @param name Reaction name
    /// @param reactant Parent molecule
    /// @param products Product description
    /// @param wavelengths Reference wavelengths [nm]
    /// @param quantum_yields Quantum yield values [0-1]
    BaseQuantumYield(
        std::string name,
        std::string reactant,
        std::string products,
        std::vector<double> wavelengths,
        std::vector<double> quantum_yields)
        : wavelengths_(std::move(wavelengths)),
          quantum_yields_(std::move(quantum_yields))
    {
      name_ = std::move(name);
      reactant_ = std::move(reactant);
      products_ = std::move(products);
    }

    /// @brief Clone this quantum yield
    std::unique_ptr<QuantumYield> Clone() const override
    {
      return std::make_unique<BaseQuantumYield>(name_, reactant_, products_, wavelengths_, quantum_yields_);
    }

    /// @brief Calculate quantum yield on wavelength grid
    std::vector<double>
    Calculate(const Grid& wavelength_grid, double /*temperature*/, double /*air_density*/ = 0.0) const override
    {
      std::size_t n_wavelengths = wavelength_grid.Spec().n_cells;

      if (wavelengths_.empty())
      {
        return std::vector<double>(n_wavelengths, 0.0);
      }

      // Interpolate to target wavelength grid
      LinearInterpolator interp;
      auto target_wavelengths = wavelength_grid.Midpoints();
      auto result = interp.Interpolate(target_wavelengths, wavelengths_, quantum_yields_);

      // Clamp to [0, 1] and zero outside reference range
      double wl_min = wavelengths_.front();
      double wl_max = wavelengths_.back();

      for (std::size_t i = 0; i < n_wavelengths; ++i)
      {
        double wl = target_wavelengths[i];
        if (wl < wl_min || wl > wl_max)
        {
          result[i] = 0.0;
        }
        else
        {
          result[i] = std::clamp(result[i], 0.0, 1.0);
        }
      }

      return result;
    }

    /// @brief Get reference wavelengths
    const std::vector<double>& ReferenceWavelengths() const
    {
      return wavelengths_;
    }

    /// @brief Get reference quantum yields
    const std::vector<double>& ReferenceQuantumYields() const
    {
      return quantum_yields_;
    }

   private:
    std::vector<double> wavelengths_;
    std::vector<double> quantum_yields_;
  };

}  // namespace tuvx
