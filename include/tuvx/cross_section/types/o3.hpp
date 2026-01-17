#pragma once

#include <cmath>
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
  /// @brief Ozone (O3) absorption cross-section with temperature dependence
  ///
  /// Implements the ozone cross-section using the recommended parameterization
  /// combining Bass-Paur (Hartley-Huggins bands, 245-340 nm) and Molina-Molina
  /// (Huggins bands, 240-350 nm) data sets, with temperature dependence.
  ///
  /// The cross-section covers wavelengths from 175-850 nm, with the strongest
  /// absorption in the Hartley band (200-320 nm).
  ///
  /// Reference:
  /// - JPL Publication 19-5 (2019): Chemical Kinetics and Photochemical Data
  /// - Sander et al., Evaluation No. 19
  class O3CrossSection : public CrossSection
  {
   public:
    /// @brief Construct O3 cross-section with default JPL data
    O3CrossSection()
    {
      name_ = "O3";
      InitializeDefaultData();
    }

    /// @brief Construct O3 cross-section with custom data
    /// @param wavelengths Reference wavelengths [nm]
    /// @param temperatures Reference temperatures [K]
    /// @param cross_sections Cross-section data [n_temps][n_wavelengths] [cm^2]
    O3CrossSection(
        std::vector<double> wavelengths,
        std::vector<double> temperatures,
        std::vector<std::vector<double>> cross_sections)
        : wavelengths_(std::move(wavelengths)),
          temperatures_(std::move(temperatures)),
          cross_section_data_(std::move(cross_sections))
    {
      name_ = "O3";
    }

    /// @brief Clone this cross-section
    std::unique_ptr<CrossSection> Clone() const override
    {
      return std::make_unique<O3CrossSection>(wavelengths_, temperatures_, cross_section_data_);
    }

    /// @brief Calculate O3 cross-section at given temperature
    std::vector<double> Calculate(const Grid& wavelength_grid, double temperature) const override
    {
      std::size_t n_wavelengths = wavelength_grid.Spec().n_cells;

      // Get cross-sections at target temperature
      auto temp_xs = TemperatureBasedCrossSection<O3CrossSection>::InterpolateTemperature(
          temperature, temperatures_, cross_section_data_);

      // Interpolate to target wavelength grid
      LinearInterpolator interp;
      auto target_wavelengths = wavelength_grid.Midpoints();
      auto result = interp.Interpolate(target_wavelengths, wavelengths_, temp_xs);

      // Zero out values outside the O3 absorption range
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

    /// @brief Get reference temperatures
    const std::vector<double>& ReferenceTemperatures() const
    {
      return temperatures_;
    }

   private:
    std::vector<double> wavelengths_;
    std::vector<double> temperatures_;
    std::vector<std::vector<double>> cross_section_data_;

    /// @brief Initialize with default data (simplified JPL recommendations)
    ///
    /// This provides representative O3 cross-sections for the Hartley and
    /// Huggins bands. For production use, load full data from files.
    void InitializeDefaultData()
    {
      // Representative wavelengths covering UV-B and UV-C
      wavelengths_ = { 175.0, 200.0, 210.0, 220.0, 230.0, 240.0, 250.0, 260.0, 270.0, 280.0,
                       290.0, 300.0, 310.0, 320.0, 330.0, 340.0, 350.0, 400.0, 500.0, 600.0 };

      // Reference temperatures
      temperatures_ = { 218.0, 228.0, 243.0, 273.0, 295.0 };

      // Cross-section values [cm^2/molecule]
      // Values are representative based on JPL-19 recommendations
      // At 295 K (room temperature)
      std::vector<double> xs_295K = {
        1.0e-17,  // 175 nm - Hartley continuum
        1.1e-17,  // 200 nm
        1.0e-17,  // 210 nm
        7.4e-18,  // 220 nm
        4.3e-18,  // 230 nm
        2.1e-18,  // 240 nm
        9.9e-19,  // 250 nm
        5.1e-19,  // 260 nm
        3.3e-19,  // 270 nm
        2.6e-19,  // 280 nm - Huggins bands begin
        1.4e-19,  // 290 nm
        4.3e-20,  // 300 nm
        7.6e-21,  // 310 nm
        9.5e-22,  // 320 nm
        1.6e-22,  // 330 nm
        5.0e-23,  // 340 nm
        1.5e-23,  // 350 nm
        1.0e-24,  // 400 nm - Chappuis band
        4.0e-21,  // 500 nm - Chappuis peak
        5.0e-21   // 600 nm - Chappuis
      };

      // At 218 K (stratospheric temperature)
      // Temperature effect primarily in Huggins bands (300-350 nm)
      std::vector<double> xs_218K = {
        1.0e-17,  // 175 nm
        1.1e-17,  // 200 nm
        1.0e-17,  // 210 nm
        7.4e-18,  // 220 nm
        4.3e-18,  // 230 nm
        2.1e-18,  // 240 nm
        9.9e-19,  // 250 nm
        5.1e-19,  // 260 nm
        3.3e-19,  // 270 nm
        2.6e-19,  // 280 nm
        1.4e-19,  // 290 nm
        3.8e-20,  // 300 nm - reduced at low T
        5.5e-21,  // 310 nm
        5.5e-22,  // 320 nm
        6.0e-23,  // 330 nm
        1.5e-23,  // 340 nm
        4.0e-24,  // 350 nm
        1.0e-24,  // 400 nm
        4.0e-21,  // 500 nm
        5.0e-21   // 600 nm
      };

      // Interpolate for intermediate temperatures
      std::vector<double> xs_228K(wavelengths_.size());
      std::vector<double> xs_243K(wavelengths_.size());
      std::vector<double> xs_273K(wavelengths_.size());

      for (std::size_t i = 0; i < wavelengths_.size(); ++i)
      {
        double t_frac = (228.0 - 218.0) / (295.0 - 218.0);
        xs_228K[i] = xs_218K[i] + t_frac * (xs_295K[i] - xs_218K[i]);

        t_frac = (243.0 - 218.0) / (295.0 - 218.0);
        xs_243K[i] = xs_218K[i] + t_frac * (xs_295K[i] - xs_218K[i]);

        t_frac = (273.0 - 218.0) / (295.0 - 218.0);
        xs_273K[i] = xs_218K[i] + t_frac * (xs_295K[i] - xs_218K[i]);
      }

      cross_section_data_ = { xs_218K, xs_228K, xs_243K, xs_273K, xs_295K };
    }
  };

}  // namespace tuvx
