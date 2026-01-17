#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace tuvx
{
  /// @brief Mixin class providing temperature interpolation for cross-sections
  ///
  /// TemperatureBasedCrossSection provides functionality for cross-sections
  /// that are tabulated at multiple reference temperatures. The cross-section
  /// at an arbitrary temperature is computed by linear interpolation between
  /// the two nearest reference temperatures.
  ///
  /// This class is designed to be used as a CRTP base class or as a utility
  /// for cross-section implementations that need temperature interpolation.
  template<typename Derived>
  class TemperatureBasedCrossSection
  {
   public:
    /// @brief Interpolate cross-sections to a target temperature
    /// @param target_temperature Temperature to interpolate to [K]
    /// @param reference_temperatures Sorted array of reference temperatures [K]
    /// @param cross_section_data Cross-sections at each reference temperature
    ///        [n_temperatures][n_wavelengths]
    /// @return Interpolated cross-sections [n_wavelengths]
    ///
    /// @note Reference temperatures must be sorted in ascending order.
    /// @note Extrapolation uses the nearest reference temperature (no extrapolation).
    static std::vector<double> InterpolateTemperature(
        double target_temperature,
        const std::vector<double>& reference_temperatures,
        const std::vector<std::vector<double>>& cross_section_data)
    {
      if (reference_temperatures.empty() || cross_section_data.empty())
      {
        return {};
      }

      std::size_t n_temps = reference_temperatures.size();
      std::size_t n_wavelengths = cross_section_data[0].size();

      // Clamp to bounds - no extrapolation
      double temp_lo = reference_temperatures.front();
      double temp_hi = reference_temperatures.back();
      double clamped_temp = std::clamp(target_temperature, temp_lo, temp_hi);

      // Find bracketing temperatures
      auto it_upper = std::lower_bound(reference_temperatures.begin(), reference_temperatures.end(), clamped_temp);

      // Handle exact match at lower bound
      if (it_upper == reference_temperatures.begin())
      {
        return cross_section_data[0];
      }

      // Handle upper bound
      if (it_upper == reference_temperatures.end())
      {
        return cross_section_data[n_temps - 1];
      }

      auto it_lower = it_upper - 1;
      std::size_t i_lower = static_cast<std::size_t>(it_lower - reference_temperatures.begin());
      std::size_t i_upper = i_lower + 1;

      double t_lower = *it_lower;
      double t_upper = *it_upper;

      // Handle exact match
      if (std::abs(t_upper - t_lower) < 1e-10)
      {
        return cross_section_data[i_lower];
      }

      // Linear interpolation weight
      double weight = (clamped_temp - t_lower) / (t_upper - t_lower);

      std::vector<double> result(n_wavelengths);
      const auto& data_lower = cross_section_data[i_lower];
      const auto& data_upper = cross_section_data[i_upper];

      for (std::size_t i = 0; i < n_wavelengths; ++i)
      {
        result[i] = data_lower[i] + weight * (data_upper[i] - data_lower[i]);
      }

      return result;
    }

    /// @brief Find the bounding indices for a temperature
    /// @param target_temperature Temperature to find bounds for [K]
    /// @param reference_temperatures Sorted array of reference temperatures
    /// @param lower_index Output: index of lower bounding temperature
    /// @param upper_index Output: index of upper bounding temperature
    /// @param weight Output: interpolation weight (0 = lower, 1 = upper)
    /// @return True if interpolation is possible, false if extrapolation needed
    static bool FindTemperatureBounds(
        double target_temperature,
        const std::vector<double>& reference_temperatures,
        std::size_t& lower_index,
        std::size_t& upper_index,
        double& weight)
    {
      if (reference_temperatures.empty())
      {
        lower_index = 0;
        upper_index = 0;
        weight = 0.0;
        return false;
      }

      std::size_t n_temps = reference_temperatures.size();

      // Below range
      if (target_temperature <= reference_temperatures.front())
      {
        lower_index = 0;
        upper_index = 0;
        weight = 0.0;
        return false;
      }

      // Above range
      if (target_temperature >= reference_temperatures.back())
      {
        lower_index = n_temps - 1;
        upper_index = n_temps - 1;
        weight = 0.0;
        return false;
      }

      // Find bracketing indices
      auto it_upper = std::lower_bound(reference_temperatures.begin(), reference_temperatures.end(), target_temperature);

      upper_index = static_cast<std::size_t>(it_upper - reference_temperatures.begin());
      lower_index = upper_index - 1;

      double t_lower = reference_temperatures[lower_index];
      double t_upper = reference_temperatures[upper_index];
      weight = (target_temperature - t_lower) / (t_upper - t_lower);

      return true;
    }
  };

}  // namespace tuvx
