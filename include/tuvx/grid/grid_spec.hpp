#pragma once

#include <cstddef>
#include <string>

namespace tuvx
{
  /// @brief Immutable specification for a 1D grid
  ///
  /// GridSpec contains the metadata that defines a grid's identity:
  /// its name, units, and size. This struct is used to create Grid
  /// and MutableGrid instances.
  struct GridSpec
  {
    /// @brief Name of the grid (e.g., "wavelength", "altitude", "time")
    std::string name{};

    /// @brief Units of the grid values (e.g., "nm", "km", "hours")
    std::string units{};

    /// @brief Number of cells in the grid
    /// @note The grid will have n_cells + 1 edges
    std::size_t n_cells{ 0 };

    /// @brief Check equality between two GridSpec objects
    /// @param other The other GridSpec to compare
    /// @return True if name, units, and n_cells are all equal
    bool operator==(const GridSpec& other) const
    {
      return name == other.name && units == other.units && n_cells == other.n_cells;
    }

    /// @brief Check inequality between two GridSpec objects
    /// @param other The other GridSpec to compare
    /// @return True if any of name, units, or n_cells differ
    bool operator!=(const GridSpec& other) const
    {
      return !(*this == other);
    }

    /// @brief Create a key string for warehouse lookup
    /// @return A string combining name and units with a pipe separator
    std::string Key() const
    {
      return name + "|" + units;
    }
  };

}  // namespace tuvx
