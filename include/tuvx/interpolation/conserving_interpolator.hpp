#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

#include <tuvx/interpolation/interpolator.hpp>

namespace tuvx
{
  /// @brief Area-conserving bin-to-bin interpolation
  ///
  /// ConservingInterpolator performs interpolation that preserves the total
  /// integral (area) of the source data when mapping to a target grid.
  /// This is essential for quantities like flux or spectral irradiance
  /// where the total integrated amount must be conserved.
  ///
  /// The interpolation works with bin edges:
  /// - source_edges: n_source + 1 edges defining source bins
  /// - source_values: n_source values (one per source bin)
  /// - target_edges: n_target + 1 edges defining target bins
  ///
  /// For each target bin, the interpolator computes the area-weighted
  /// average of overlapping source bins.
  class ConservingInterpolator
  {
   public:
    /// @brief Construct a conserving interpolator with default settings
    ConservingInterpolator() = default;

    /// @brief Interpolate bin data from source to target grid
    /// @param target_edges Target bin edges (n_target + 1 values)
    /// @param source_edges Source bin edges (n_source + 1 values, must be sorted ascending)
    /// @param source_values Source bin values (n_source values)
    /// @return Vector of n_target values for target bins
    ///
    /// @note Source and target edges must be sorted in ascending order.
    /// @note The number of values equals number of edges minus one.
    /// @note Target bins outside source range get zero values.
    std::vector<double> Interpolate(
        std::span<const double> target_edges,
        std::span<const double> source_edges,
        std::span<const double> source_values) const
    {
      std::vector<double> result;

      // Number of target bins
      if (target_edges.size() < 2)
      {
        return result;
      }
      std::size_t n_target = target_edges.size() - 1;
      result.reserve(n_target);

      // Validate source data
      if (source_edges.size() < 2 || source_values.size() != source_edges.size() - 1)
      {
        result.resize(n_target, 0.0);
        return result;
      }

      for (std::size_t i = 0; i < n_target; ++i)
      {
        double target_lo = target_edges[i];
        double target_hi = target_edges[i + 1];
        double target_width = target_hi - target_lo;

        if (target_width <= 0)
        {
          result.push_back(0.0);
          continue;
        }

        double accumulated_area = 0.0;

        // Find overlapping source bins
        for (std::size_t j = 0; j < source_values.size(); ++j)
        {
          double source_lo = source_edges[j];
          double source_hi = source_edges[j + 1];

          // Calculate overlap region
          double overlap_lo = std::max(target_lo, source_lo);
          double overlap_hi = std::min(target_hi, source_hi);
          double overlap_width = overlap_hi - overlap_lo;

          if (overlap_width > 0)
          {
            // Area contribution = source_value * overlap_width
            accumulated_area += source_values[j] * overlap_width;
          }
        }

        // Target bin value = total area / target_width
        result.push_back(accumulated_area / target_width);
      }

      return result;
    }

    /// @brief Check if extrapolation is supported
    /// @return False (conserving interpolator returns zero outside source range)
    bool CanExtrapolate() const
    {
      return false;
    }
  };

  // Verify ConservingInterpolator satisfies the Interpolator concept
  static_assert(Interpolator<ConservingInterpolator>);
  static_assert(ExtendedInterpolator<ConservingInterpolator>);

}  // namespace tuvx
