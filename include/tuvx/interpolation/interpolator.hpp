#pragma once

#include <concepts>
#include <span>
#include <vector>

namespace tuvx
{
  /// @brief Concept for interpolation strategies
  ///
  /// Interpolator defines the interface for classes that interpolate
  /// source data onto target coordinates. Implementations can provide
  /// different interpolation methods (linear, conserving, spline, etc.)
  ///
  /// @tparam T Type being tested against the concept
  template<typename T>
  concept Interpolator = requires(T interpolator,
                                  std::span<const double> target_x,
                                  std::span<const double> source_x,
                                  std::span<const double> source_y) {
    /// Interpolate source data (source_x, source_y) onto target coordinates.
    /// Returns a vector with one value for each point in target_x.
    { interpolator.Interpolate(target_x, source_x, source_y) } -> std::same_as<std::vector<double>>;
  };

  /// @brief Extended interpolator concept with additional capabilities
  ///
  /// ExtendedInterpolator adds capabilities for handling out-of-range
  /// behavior and querying interpolator properties.
  ///
  /// @tparam T Type being tested against the concept
  template<typename T>
  concept ExtendedInterpolator = Interpolator<T> && requires(const T interpolator) {
    /// Returns true if the interpolator can extrapolate beyond source bounds
    { interpolator.CanExtrapolate() } -> std::same_as<bool>;
  };

}  // namespace tuvx
