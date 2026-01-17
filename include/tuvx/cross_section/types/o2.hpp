#pragma once

#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <tuvx/cross_section/cross_section.hpp>
#include <tuvx/interpolation/linear_interpolator.hpp>

namespace tuvx
{
  /// @brief Oxygen (O2) absorption cross-section
  ///
  /// Implements the O2 cross-section covering:
  /// - Schumann-Runge continuum (130-175 nm)
  /// - Schumann-Runge bands (175-205 nm)
  /// - Herzberg continuum (195-240 nm)
  ///
  /// The cross-section has weak temperature dependence in the Schumann-Runge
  /// bands region. This implementation uses representative values from
  /// JPL Publication 19-5.
  ///
  /// Reference:
  /// - JPL Publication 19-5 (2019): Chemical Kinetics and Photochemical Data
  /// - Yoshino et al. (1992) for Schumann-Runge bands
  class O2CrossSection : public CrossSection
  {
   public:
    /// @brief Construct O2 cross-section with default JPL data
    O2CrossSection()
    {
      name_ = "O2";
      InitializeDefaultData();
    }

    /// @brief Construct O2 cross-section with custom data
    /// @param wavelengths Reference wavelengths [nm]
    /// @param cross_sections Cross-section values [cm^2]
    O2CrossSection(std::vector<double> wavelengths, std::vector<double> cross_sections)
        : wavelengths_(std::move(wavelengths)),
          cross_section_data_(std::move(cross_sections))
    {
      name_ = "O2";
    }

    /// @brief Clone this cross-section
    std::unique_ptr<CrossSection> Clone() const override
    {
      return std::make_unique<O2CrossSection>(wavelengths_, cross_section_data_);
    }

    /// @brief Calculate O2 cross-section (temperature-independent approximation)
    std::vector<double> Calculate(const Grid& wavelength_grid, double temperature) const override
    {
      (void)temperature;  // Temperature dependence is weak, ignored in this version

      std::size_t n_wavelengths = wavelength_grid.Spec().n_cells;

      // Interpolate to target wavelength grid
      LinearInterpolator interp;
      auto target_wavelengths = wavelength_grid.Midpoints();
      auto result = interp.Interpolate(target_wavelengths, wavelengths_, cross_section_data_);

      // Zero out values outside the O2 absorption range
      double wl_min = wavelengths_.front();
      double wl_max = wavelengths_.back();

      for (std::size_t i = 0; i < n_wavelengths; ++i)
      {
        double wl = target_wavelengths[i];
        if (wl < wl_min || wl > wl_max)
        {
          result[i] = 0.0;
        }
        // Ensure non-negative
        if (result[i] < 0.0)
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

   private:
    std::vector<double> wavelengths_;
    std::vector<double> cross_section_data_;

    /// @brief Initialize with default data
    ///
    /// Representative O2 cross-sections covering UV-C region.
    /// The Schumann-Runge bands (175-205 nm) show complex structure
    /// that is simplified here to representative continuum values.
    void InitializeDefaultData()
    {
      // Wavelengths from far UV to edge of UV-C [nm]
      wavelengths_ = {
        130.0, 140.0, 150.0, 160.0, 170.0,  // Schumann-Runge continuum
        175.0, 180.0, 185.0, 190.0, 195.0,  // Schumann-Runge bands
        200.0, 205.0, 210.0, 215.0, 220.0,  // Herzberg continuum
        225.0, 230.0, 235.0, 240.0, 245.0
      };

      // Cross-section values [cm^2/molecule]
      // Based on JPL-19 recommendations
      cross_section_data_ = {
        1.5e-17,  // 130 nm - SR continuum (strong)
        1.2e-17,  // 140 nm
        8.0e-18,  // 150 nm
        4.0e-18,  // 160 nm
        1.5e-18,  // 170 nm
        7.0e-19,  // 175 nm - SR bands begin
        3.0e-19,  // 180 nm
        1.5e-19,  // 185 nm
        8.0e-20,  // 190 nm
        4.0e-20,  // 195 nm
        1.5e-20,  // 200 nm
        5.0e-21,  // 205 nm - SR bands end
        1.5e-21,  // 210 nm - Herzberg continuum
        7.0e-22,  // 215 nm
        3.0e-22,  // 220 nm
        1.0e-22,  // 225 nm
        5.0e-23,  // 230 nm
        2.0e-23,  // 235 nm
        1.0e-23,  // 240 nm
        5.0e-24   // 245 nm
      };
    }
  };

}  // namespace tuvx
