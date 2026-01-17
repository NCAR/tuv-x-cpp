#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include <tuvx/grid/grid.hpp>
#include <tuvx/interpolation/linear_interpolator.hpp>

namespace tuvx
{
  /// @brief Surface reflectivity (albedo) specification
  ///
  /// Surface albedo affects the upwelling diffuse radiation and can vary
  /// significantly with wavelength. This class provides wavelength-dependent
  /// albedo values for use in radiative transfer calculations.
  ///
  /// Typical albedo values:
  /// - Ocean (calm): 0.03-0.06
  /// - Ocean (rough): 0.06-0.10
  /// - Fresh snow: 0.80-0.95
  /// - Old snow: 0.40-0.70
  /// - Sand: 0.30-0.40
  /// - Grass: 0.15-0.25
  /// - Forest: 0.10-0.20
  /// - Urban: 0.10-0.20
  class SurfaceAlbedo
  {
   public:
    /// @brief Construct a wavelength-independent (gray) albedo
    /// @param albedo Constant albedo value [0, 1]
    explicit SurfaceAlbedo(double albedo) : constant_albedo_(albedo), is_constant_(true)
    {
      ValidateAlbedo(albedo);
    }

    /// @brief Construct a wavelength-dependent albedo
    /// @param wavelengths Reference wavelengths [nm]
    /// @param albedo Albedo values at each wavelength [0, 1]
    SurfaceAlbedo(std::vector<double> wavelengths, std::vector<double> albedo)
        : wavelengths_(std::move(wavelengths)), albedo_(std::move(albedo)), is_constant_(false)
    {
      if (wavelengths_.size() != albedo_.size())
      {
        throw std::invalid_argument("Wavelength and albedo arrays must have same size");
      }
      if (wavelengths_.size() < 2)
      {
        throw std::invalid_argument("At least 2 wavelength points required for spectral albedo");
      }

      for (double a : albedo_)
      {
        ValidateAlbedo(a);
      }
    }

    /// @brief Calculate albedo values on a wavelength grid
    /// @param wavelength_grid Target wavelength grid
    /// @return Albedo values at grid midpoints
    std::vector<double> Calculate(const Grid& wavelength_grid) const
    {
      std::size_t n_wavelengths = wavelength_grid.Spec().n_cells;

      if (is_constant_)
      {
        return std::vector<double>(n_wavelengths, constant_albedo_);
      }

      auto target_wavelengths = wavelength_grid.Midpoints();

      LinearInterpolator interp;
      std::vector<double> result = interp.Interpolate(target_wavelengths, wavelengths_, albedo_);

      // Extrapolate using edge values for wavelengths outside range
      double wl_min = wavelengths_.front();
      double wl_max = wavelengths_.back();
      double albedo_min = albedo_.front();
      double albedo_max = albedo_.back();

      for (std::size_t i = 0; i < n_wavelengths; ++i)
      {
        if (target_wavelengths[i] < wl_min)
        {
          result[i] = albedo_min;  // Use lowest wavelength value
        }
        else if (target_wavelengths[i] > wl_max)
        {
          result[i] = albedo_max;  // Use highest wavelength value
        }

        // Clamp to valid range
        result[i] = std::clamp(result[i], 0.0, 1.0);
      }

      return result;
    }

    /// @brief Get albedo at a single wavelength
    /// @param wavelength Wavelength [nm]
    /// @return Albedo value
    double At(double wavelength) const
    {
      if (is_constant_)
      {
        return constant_albedo_;
      }

      // Simple linear interpolation for single point
      if (wavelength <= wavelengths_.front())
      {
        return albedo_.front();
      }
      if (wavelength >= wavelengths_.back())
      {
        return albedo_.back();
      }

      // Find bracketing points
      auto it = std::lower_bound(wavelengths_.begin(), wavelengths_.end(), wavelength);
      std::size_t idx = static_cast<std::size_t>(std::distance(wavelengths_.begin(), it));

      if (idx == 0)
      {
        return albedo_[0];
      }

      // Linear interpolation
      double wl_lo = wavelengths_[idx - 1];
      double wl_hi = wavelengths_[idx];
      double a_lo = albedo_[idx - 1];
      double a_hi = albedo_[idx];

      double t = (wavelength - wl_lo) / (wl_hi - wl_lo);
      return a_lo + t * (a_hi - a_lo);
    }

    /// @brief Check if albedo is wavelength-independent
    bool IsConstant() const
    {
      return is_constant_;
    }

    /// @brief Get constant albedo value (only valid if IsConstant() is true)
    double ConstantValue() const
    {
      if (!is_constant_)
      {
        throw std::runtime_error("Cannot get constant value from spectral albedo");
      }
      return constant_albedo_;
    }

    /// @brief Get reference wavelengths (only valid if IsConstant() is false)
    const std::vector<double>& ReferenceWavelengths() const
    {
      if (is_constant_)
      {
        throw std::runtime_error("Constant albedo has no reference wavelengths");
      }
      return wavelengths_;
    }

    /// @brief Get reference albedo values (only valid if IsConstant() is false)
    const std::vector<double>& ReferenceAlbedo() const
    {
      if (is_constant_)
      {
        throw std::runtime_error("Constant albedo has no reference values");
      }
      return albedo_;
    }

   private:
    void ValidateAlbedo(double albedo)
    {
      if (albedo < 0.0 || albedo > 1.0)
      {
        throw std::invalid_argument("Albedo must be in range [0, 1]");
      }
    }

    std::vector<double> wavelengths_;
    std::vector<double> albedo_;
    double constant_albedo_{ 0.0 };
    bool is_constant_{ false };
  };

  /// @brief Standard surface albedo profiles
  namespace surface_types
  {
    /// @brief Create ocean surface albedo (low, wavelength-independent)
    /// @param wind_speed Wind speed [m/s] - higher wind = rougher surface = higher albedo
    /// @return Ocean surface albedo
    inline SurfaceAlbedo Ocean(double wind_speed = 5.0)
    {
      // Simple parameterization: calm ocean ~0.03, rough ~0.10
      double albedo = 0.03 + 0.007 * std::min(wind_speed, 10.0);
      return SurfaceAlbedo(albedo);
    }

    /// @brief Create fresh snow surface albedo (high, wavelength-dependent)
    /// @return Fresh snow albedo
    inline SurfaceAlbedo FreshSnow()
    {
      // Snow albedo decreases in near-IR
      std::vector<double> wavelengths = { 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 1000.0 };
      std::vector<double> albedo = { 0.95, 0.95, 0.90, 0.85, 0.80, 0.70, 0.50 };
      return SurfaceAlbedo(wavelengths, albedo);
    }

    /// @brief Create desert/sand surface albedo
    /// @return Desert surface albedo
    inline SurfaceAlbedo Desert()
    {
      // Sand has relatively high albedo in visible, lower in UV
      std::vector<double> wavelengths = { 300.0, 400.0, 500.0, 600.0, 700.0, 800.0 };
      std::vector<double> albedo = { 0.10, 0.20, 0.30, 0.35, 0.40, 0.40 };
      return SurfaceAlbedo(wavelengths, albedo);
    }

    /// @brief Create vegetation/grass surface albedo
    /// @return Vegetation surface albedo
    inline SurfaceAlbedo Vegetation()
    {
      // Vegetation has low albedo in visible (chlorophyll absorption), higher in near-IR
      std::vector<double> wavelengths = { 300.0, 400.0, 500.0, 550.0, 600.0, 700.0, 800.0 };
      std::vector<double> albedo = { 0.05, 0.05, 0.10, 0.15, 0.10, 0.30, 0.50 };
      return SurfaceAlbedo(wavelengths, albedo);
    }

    /// @brief Create urban surface albedo
    /// @return Urban surface albedo
    inline SurfaceAlbedo Urban()
    {
      return SurfaceAlbedo(0.15);  // Typical gray value
    }

    /// @brief Create forest surface albedo
    /// @return Forest surface albedo
    inline SurfaceAlbedo Forest()
    {
      // Dark in visible, reflective in near-IR
      std::vector<double> wavelengths = { 300.0, 400.0, 500.0, 600.0, 700.0, 800.0 };
      std::vector<double> albedo = { 0.03, 0.03, 0.05, 0.05, 0.20, 0.40 };
      return SurfaceAlbedo(wavelengths, albedo);
    }
  }  // namespace surface_types

}  // namespace tuvx
