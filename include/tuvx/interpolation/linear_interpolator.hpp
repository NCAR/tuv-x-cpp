#pragma once

#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

#include <tuvx/interpolation/interpolator.hpp>

namespace tuvx
{
  /// @brief Point-to-point linear interpolation
  ///
  /// LinearInterpolator performs linear interpolation between data points.
  /// For each target x-coordinate, it finds the bracketing source points
  /// and linearly interpolates the y-value.
  ///
  /// Out-of-range behavior:
  /// - Points below the source range return the first source y-value
  /// - Points above the source range return the last source y-value
  /// - (No extrapolation by default)
  class LinearInterpolator
  {
   public:
    /// @brief Construct a linear interpolator with default settings
    LinearInterpolator() = default;

    /// @brief Interpolate source data onto target coordinates
    /// @param target_x Target x-coordinates where values are needed
    /// @param source_x Source x-coordinates (must be sorted ascending)
    /// @param source_y Source y-values corresponding to source_x
    /// @return Vector of interpolated y-values at target_x positions
    ///
    /// @note Source data must be sorted in ascending x order.
    /// @note For points outside source range, boundary values are returned.
    std::vector<double> Interpolate(
        std::span<const double> target_x,
        std::span<const double> source_x,
        std::span<const double> source_y) const
    {
      std::vector<double> result;
      result.reserve(target_x.size());

      if (source_x.empty() || source_y.empty())
      {
        result.resize(target_x.size(), 0.0);
        return result;
      }

      if (source_x.size() != source_y.size())
      {
        result.resize(target_x.size(), 0.0);
        return result;
      }

      for (const double x : target_x)
      {
        result.push_back(InterpolateSingle(x, source_x, source_y));
      }

      return result;
    }

    /// @brief Check if extrapolation is supported
    /// @return False (linear interpolator returns boundary values, not extrapolation)
    bool CanExtrapolate() const
    {
      return false;
    }

   private:
    /// @brief Interpolate a single point
    double InterpolateSingle(
        double x,
        std::span<const double> source_x,
        std::span<const double> source_y) const
    {
      // Handle boundary cases
      if (x <= source_x.front())
      {
        return source_y.front();
      }
      if (x >= source_x.back())
      {
        return source_y.back();
      }

      // Binary search for bracketing interval
      auto it = std::upper_bound(source_x.begin(), source_x.end(), x);
      std::size_t i_upper = static_cast<std::size_t>(std::distance(source_x.begin(), it));
      std::size_t i_lower = i_upper - 1;

      // Linear interpolation
      double x0 = source_x[i_lower];
      double x1 = source_x[i_upper];
      double y0 = source_y[i_lower];
      double y1 = source_y[i_upper];

      double t = (x - x0) / (x1 - x0);
      return y0 + t * (y1 - y0);
    }
  };

  // Verify LinearInterpolator satisfies the Interpolator concept
  static_assert(Interpolator<LinearInterpolator>);
  static_assert(ExtendedInterpolator<LinearInterpolator>);

}  // namespace tuvx
