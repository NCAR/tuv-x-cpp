#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace tuvx
{
  namespace array
  {
    /// @brief Compare two floating-point numbers for approximate equality
    /// @param a First value
    /// @param b Second value
    /// @param rel_tol Relative tolerance (default 1e-9)
    /// @param abs_tol Absolute tolerance (default 1e-12)
    /// @return True if values are approximately equal
    inline bool AlmostEqual(double a, double b, double rel_tol = 1e-9, double abs_tol = 1e-12)
    {
      // Handle NaN
      if (std::isnan(a) || std::isnan(b))
      {
        return false;
      }

      // Handle infinities
      if (std::isinf(a) || std::isinf(b))
      {
        return a == b;
      }

      // Handle exact equality (including both zeros)
      if (a == b)
      {
        return true;
      }

      // Relative and absolute tolerance check
      double diff = std::abs(a - b);
      double max_val = std::max(std::abs(a), std::abs(b));
      return diff <= std::max(rel_tol * max_val, abs_tol);
    }

    /// @brief Find a string in a vector of strings
    /// @param array Vector to search
    /// @param target String to find
    /// @param case_sensitive Whether to perform case-sensitive comparison (default false)
    /// @return Index of the found string, or std::nullopt if not found
    inline std::optional<std::size_t> FindString(
        const std::vector<std::string>& array,
        const std::string& target,
        bool case_sensitive = false)
    {
      auto compare = [case_sensitive](const std::string& a, const std::string& b) -> bool
      {
        if (case_sensitive)
        {
          return a == b;
        }
        if (a.size() != b.size())
        {
          return false;
        }
        for (std::size_t i = 0; i < a.size(); ++i)
        {
          if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
          {
            return false;
          }
        }
        return true;
      };

      for (std::size_t i = 0; i < array.size(); ++i)
      {
        if (compare(array[i], target))
        {
          return i;
        }
      }
      return std::nullopt;
    }

    /// @brief Generate a linearly spaced array
    /// @param min Minimum value (first element)
    /// @param max Maximum value (last element)
    /// @param n Number of elements (must be >= 2)
    /// @return Vector of linearly spaced values
    inline std::vector<double> Linspace(double min, double max, std::size_t n)
    {
      std::vector<double> result;
      if (n == 0)
      {
        return result;
      }
      if (n == 1)
      {
        result.push_back(min);
        return result;
      }

      result.reserve(n);
      double step = (max - min) / static_cast<double>(n - 1);
      for (std::size_t i = 0; i < n; ++i)
      {
        result.push_back(min + static_cast<double>(i) * step);
      }
      // Ensure exact endpoint
      result.back() = max;
      return result;
    }

    /// @brief Generate a logarithmically spaced array
    /// @param min Minimum value (first element, must be positive)
    /// @param max Maximum value (last element, must be positive)
    /// @param n Number of elements (must be >= 2)
    /// @return Vector of logarithmically spaced values
    inline std::vector<double> Logspace(double min, double max, std::size_t n)
    {
      std::vector<double> result;
      if (n == 0 || min <= 0 || max <= 0)
      {
        return result;
      }
      if (n == 1)
      {
        result.push_back(min);
        return result;
      }

      result.reserve(n);
      double log_min = std::log(min);
      double log_max = std::log(max);
      double log_step = (log_max - log_min) / static_cast<double>(n - 1);

      for (std::size_t i = 0; i < n; ++i)
      {
        result.push_back(std::exp(log_min + static_cast<double>(i) * log_step));
      }
      // Ensure exact endpoints
      result.front() = min;
      result.back() = max;
      return result;
    }

    /// @brief Merge two sorted arrays without duplicates
    /// @param a First sorted array
    /// @param b Second sorted array
    /// @param tolerance Tolerance for duplicate detection (default 1e-10)
    /// @return Merged sorted array without duplicates
    inline std::vector<double> MergeSorted(const std::vector<double>& a, const std::vector<double>& b, double tolerance = 1e-10)
    {
      std::vector<double> result;
      result.reserve(a.size() + b.size());

      std::size_t i = 0;
      std::size_t j = 0;

      while (i < a.size() && j < b.size())
      {
        if (AlmostEqual(a[i], b[j], tolerance, tolerance))
        {
          // Values are considered equal, take one and advance both
          result.push_back(a[i]);
          ++i;
          ++j;
        }
        else if (a[i] < b[j])
        {
          result.push_back(a[i]);
          ++i;
        }
        else
        {
          result.push_back(b[j]);
          ++j;
        }
      }

      // Add remaining elements
      while (i < a.size())
      {
        result.push_back(a[i]);
        ++i;
      }
      while (j < b.size())
      {
        result.push_back(b[j]);
        ++j;
      }

      return result;
    }

  }  // namespace array
}  // namespace tuvx
