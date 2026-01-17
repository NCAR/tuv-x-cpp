#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <tuvx/cross_section/cross_section.hpp>
#include <tuvx/cross_section/temperature_based.hpp>
#include <tuvx/interpolation/linear_interpolator.hpp>

namespace tuvx
{
  /// @brief Simple lookup table cross-section with optional temperature dependence
  ///
  /// BaseCrossSection provides a straightforward implementation that stores
  /// cross-section data at one or more reference temperatures and interpolates
  /// to the target wavelength grid and temperature.
  ///
  /// For temperature-independent cross-sections, provide data at a single
  /// reference temperature. For temperature-dependent cross-sections, provide
  /// data at multiple reference temperatures.
  class BaseCrossSection : public CrossSection
  {
   public:
    /// @brief Construct a temperature-independent cross-section
    /// @param name Species or reaction name
    /// @param wavelengths Reference wavelengths [nm]
    /// @param cross_sections Cross-section values [cm^2/molecule]
    BaseCrossSection(std::string name, std::vector<double> wavelengths, std::vector<double> cross_sections)
        : wavelengths_(std::move(wavelengths)),
          reference_temperatures_{ 298.0 },
          cross_section_data_{ std::move(cross_sections) }
    {
      name_ = std::move(name);
    }

    /// @brief Construct a temperature-dependent cross-section
    /// @param name Species or reaction name
    /// @param wavelengths Reference wavelengths [nm]
    /// @param temperatures Reference temperatures [K], must be sorted ascending
    /// @param cross_sections Cross-section data [n_temperatures][n_wavelengths]
    BaseCrossSection(
        std::string name,
        std::vector<double> wavelengths,
        std::vector<double> temperatures,
        std::vector<std::vector<double>> cross_sections)
        : wavelengths_(std::move(wavelengths)),
          reference_temperatures_(std::move(temperatures)),
          cross_section_data_(std::move(cross_sections))
    {
      name_ = std::move(name);
    }

    /// @brief Clone this cross-section
    std::unique_ptr<CrossSection> Clone() const override
    {
      auto clone = std::make_unique<BaseCrossSection>(name_, wavelengths_, reference_temperatures_, cross_section_data_);
      return clone;
    }

    /// @brief Calculate cross-section values on wavelength grid
    std::vector<double> Calculate(const Grid& wavelength_grid, double temperature) const override
    {
      std::size_t n_wavelengths = wavelength_grid.Spec().n_cells;
      std::vector<double> result(n_wavelengths);

      // Get cross-sections at target temperature (interpolated if needed)
      std::vector<double> temp_xs;
      if (reference_temperatures_.size() == 1)
      {
        temp_xs = cross_section_data_[0];
      }
      else
      {
        temp_xs = TemperatureBasedCrossSection<BaseCrossSection>::InterpolateTemperature(
            temperature, reference_temperatures_, cross_section_data_);
      }

      // Interpolate to target wavelength grid
      LinearInterpolator interp;
      auto target_wavelengths = wavelength_grid.Midpoints();

      result = interp.Interpolate(target_wavelengths, wavelengths_, temp_xs);

      // Zero out values outside the reference wavelength range
      double wl_min = wavelengths_.front();
      double wl_max = wavelengths_.back();

      for (std::size_t i = 0; i < n_wavelengths; ++i)
      {
        double wl = target_wavelengths[i];
        if (wl < wl_min || wl > wl_max)
        {
          result[i] = 0.0;
        }
      }

      return result;
    }

    /// @brief Get reference wavelengths
    const std::vector<double>& ReferenceWavelengths() const
    {
      return wavelengths_;
    }

    /// @brief Get reference temperatures
    const std::vector<double>& ReferenceTemperatures() const
    {
      return reference_temperatures_;
    }

    /// @brief Check if this cross-section is temperature-dependent
    bool IsTemperatureDependent() const
    {
      return reference_temperatures_.size() > 1;
    }

   private:
    std::vector<double> wavelengths_;
    std::vector<double> reference_temperatures_;
    std::vector<std::vector<double>> cross_section_data_;
  };

}  // namespace tuvx
