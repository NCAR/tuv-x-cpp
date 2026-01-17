#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include <tuvx/grid/grid.hpp>
#include <tuvx/util/constants.hpp>

namespace tuvx
{
  /// @brief Spherical geometry calculations for radiative transfer
  ///
  /// Computes slant paths through a spherical atmosphere, accounting for
  /// Earth's curvature. This is essential for accurate radiative transfer
  /// at high solar zenith angles (> 70°) and for twilight conditions.
  ///
  /// Based on: Dahlback, A. and K. Stamnes, A new spherical model for computing
  /// the radiation field available for photolysis and heating at twilight,
  /// Planet. Space Sci., v39, n5, pp 671-683, 1991.
  class SphericalGeometry
  {
   public:
    /// @brief Construct for a given altitude grid
    /// @param altitude_grid Altitude grid with edges in km
    /// @param earth_radius Earth radius in km (default: 6371 km)
    explicit SphericalGeometry(const Grid& altitude_grid, double earth_radius = 6371.0)
        : earth_radius_(earth_radius)
    {
      auto edges = altitude_grid.Edges();
      n_levels_ = edges.size();

      // Store radii at each level (Earth radius + altitude)
      radii_.resize(n_levels_);
      for (std::size_t i = 0; i < n_levels_; ++i)
      {
        radii_[i] = earth_radius_ + edges[i];
      }
    }

    /// @brief Calculate slant path enhancement factors for a solar zenith angle
    /// @param solar_zenith_angle Solar zenith angle [degrees]
    /// @return SlantPathResult containing enhancement factors and related info
    ///
    /// The slant path enhancement factor ds/dz gives the ratio of the
    /// actual path length through a layer to its vertical thickness.
    /// For a plane-parallel atmosphere, this is simply 1/cos(SZA).
    struct SlantPathResult
    {
      /// Slant path enhancement factor for each layer [n_layers]
      /// ds/dz = actual_path / vertical_thickness
      std::vector<double> enhancement_factor;

      /// Air mass factor at each layer [n_layers]
      /// Cumulative slant path from TOA to layer bottom
      std::vector<double> air_mass;

      /// Whether direct solar beam reaches each layer [n_layers]
      /// False if layer is in geometric shadow
      std::vector<bool> sunlit;

      /// Solar zenith angle used [degrees]
      double zenith_angle;

      /// Screening height: altitude below which surface is in shadow [km]
      /// Only meaningful when SZA > 90°
      double screening_height;
    };

    SlantPathResult Calculate(double solar_zenith_angle) const
    {
      SlantPathResult result;
      result.zenith_angle = solar_zenith_angle;

      std::size_t n_layers = n_levels_ - 1;
      result.enhancement_factor.resize(n_layers);
      result.air_mass.resize(n_layers);
      result.sunlit.resize(n_layers, true);
      result.screening_height = 0.0;

      double sza_rad = solar_zenith_angle * constants::kDegreesToRadians;
      double cos_sza = std::cos(sza_rad);
      double sin_sza = std::sin(sza_rad);

      // For SZA < ~85°, plane-parallel is a good approximation
      if (solar_zenith_angle < 85.0)
      {
        double sec_sza = 1.0 / cos_sza;

        for (std::size_t i = 0; i < n_layers; ++i)
        {
          result.enhancement_factor[i] = sec_sza;
          result.sunlit[i] = true;
        }

        // Calculate air mass (cumulative from TOA)
        result.air_mass[n_layers - 1] = sec_sza;
        for (std::size_t i = n_layers - 1; i > 0; --i)
        {
          result.air_mass[i - 1] = result.air_mass[i] + sec_sza;
        }

        return result;
      }

      // Spherical geometry for large SZA
      // Calculate screening height for twilight (SZA > 90)
      if (solar_zenith_angle > 90.0)
      {
        // Geometric screening height: where tangent ray from sun grazes Earth
        // h_s = R * (1/cos(SZA - 90) - 1) = R * (1/sin(SZA) - 1) approximately
        // More precisely: h_s = R * (sin(SZA) - 1) / (1 - sin^2(SZA))
        // Simplified: h_s ≈ R * (1 - cos(SZA - 90°))
        double tan_sza = std::tan(sza_rad);
        result.screening_height = earth_radius_ * (1.0 / std::abs(cos_sza) - 1.0);

        // Limit screening height to reasonable values
        result.screening_height = std::min(result.screening_height, radii_.back() - earth_radius_);
      }

      // Calculate slant paths using Chapman function approach
      for (std::size_t i = 0; i < n_layers; ++i)
      {
        // Layer goes from radii_[i] (bottom) to radii_[i+1] (top)
        double r_bottom = radii_[i];
        double r_top = radii_[i + 1];
        double r_mid = 0.5 * (r_bottom + r_top);

        // Check if layer is sunlit
        double h_layer = r_bottom - earth_radius_;
        if (solar_zenith_angle > 90.0 && h_layer < result.screening_height)
        {
          result.sunlit[i] = false;
          result.enhancement_factor[i] = 0.0;
          continue;
        }

        // Spherical shell enhancement factor
        // For a spherical shell, the enhancement depends on position in atmosphere
        result.enhancement_factor[i] = CalculateEnhancementFactor(r_mid, sza_rad);
      }

      // Calculate air mass (cumulative slant path from TOA)
      // Start from top of atmosphere
      double cumulative = 0.0;
      for (std::size_t i = n_layers; i > 0; --i)
      {
        std::size_t layer = i - 1;
        if (result.sunlit[layer])
        {
          double dz = radii_[layer + 1] - radii_[layer];
          cumulative += result.enhancement_factor[layer] * dz;
        }
        result.air_mass[layer] = cumulative;
      }

      return result;
    }

    /// @brief Get plane-parallel air mass approximation
    /// @param solar_zenith_angle Solar zenith angle [degrees]
    /// @return Air mass factor (approximately 1/cos(SZA))
    ///
    /// Uses Kasten-Young formula for better accuracy at high SZA.
    static double PlaneParallelAirMass(double solar_zenith_angle)
    {
      if (solar_zenith_angle >= 90.0)
      {
        return std::numeric_limits<double>::infinity();
      }

      double sza_rad = solar_zenith_angle * constants::kDegreesToRadians;

      // Simple secant for low SZA
      if (solar_zenith_angle < 75.0)
      {
        return 1.0 / std::cos(sza_rad);
      }

      // Kasten-Young formula (1989) for better accuracy at high SZA
      // AM = 1 / (cos(z) + 0.50572 * (96.07995 - z)^(-1.6364))
      double a = 0.50572;
      double b = 96.07995 - solar_zenith_angle;
      double c = -1.6364;

      return 1.0 / (std::cos(sza_rad) + a * std::pow(b, c));
    }

    /// @brief Get number of levels
    std::size_t NumberOfLevels() const
    {
      return n_levels_;
    }

    /// @brief Get radii at each level
    const std::vector<double>& Radii() const
    {
      return radii_;
    }

    /// @brief Get Earth radius used
    double EarthRadius() const
    {
      return earth_radius_;
    }

   private:
    /// Calculate enhancement factor at a given radius and SZA
    double CalculateEnhancementFactor(double radius, double sza_rad) const
    {
      double cos_sza = std::cos(sza_rad);
      double sin_sza = std::sin(sza_rad);

      // For low SZA, use simple secant
      if (std::abs(cos_sza) > 0.2)
      {
        // First-order spherical correction
        double x = (radius - earth_radius_) / earth_radius_;
        double correction = 1.0 + x * sin_sza * sin_sza;
        return correction / std::abs(cos_sza);
      }

      // For high SZA, use full spherical calculation
      // Based on Chapman function
      double y = radius / earth_radius_;

      // Approximate enhancement using geometric path
      // Path through shell at grazing angle
      if (cos_sza > 0)
      {
        // Sun above horizon
        double enhancement = std::sqrt(1.0 + (y * y - 1.0) / (cos_sza * cos_sza));
        return std::min(enhancement, 40.0);  // Cap at 40 to avoid numerical issues
      }
      else
      {
        // Sun below horizon (twilight) - complex geometry
        // Simplified: use approximate grazing angle formula
        double grazing_angle = std::acos(earth_radius_ / radius);
        double effective_sza = sza_rad - grazing_angle;

        if (effective_sza > 0 && effective_sza < constants::kHalfPi)
        {
          return 1.0 / std::cos(effective_sza);
        }
        return 0.0;  // In shadow
      }
    }

    double earth_radius_;
    std::size_t n_levels_;
    std::vector<double> radii_;
  };

  /// @brief Calculate Chapman function for spherical atmosphere
  /// @param x Dimensionless altitude parameter (r - R_earth) / H
  /// @param chi Solar zenith angle [radians]
  /// @return Chapman function value
  ///
  /// The Chapman function gives the effective path length through
  /// an exponential atmosphere at a given SZA.
  inline double ChapmanFunction(double x, double chi)
  {
    double cos_chi = std::cos(chi);

    if (cos_chi > 0.2)
    {
      // High sun - simple approximation
      return std::exp(-x) / cos_chi;
    }

    // Low sun - use Chapman function approximation
    // Ch(x, chi) ≈ sqrt(pi * x / 2) * erfc(sqrt(x) * |cos(chi)|)
    // Simplified for practical use
    double y = std::sqrt(x);

    if (cos_chi > 0)
    {
      // Above horizon
      double arg = y * cos_chi;
      return std::sqrt(constants::kPi * x / 2.0) * std::erfc(arg) * std::exp(x * cos_chi * cos_chi);
    }
    else
    {
      // Below horizon - twilight
      double sin_chi = std::sin(chi);
      return 2.0 * std::sqrt(constants::kPi * x / 2.0) * std::exp(x * (1.0 - sin_chi));
    }
  }

}  // namespace tuvx
