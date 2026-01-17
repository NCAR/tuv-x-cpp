#pragma once

#include <cstddef>
#include <string>

namespace tuvx
{
  /// @brief Immutable specification for an atmospheric profile
  ///
  /// ProfileSpec contains the metadata that defines a profile's identity:
  /// its name, units, size, and scale height for extrapolation. This struct
  /// is used to create Profile and MutableProfile instances.
  struct ProfileSpec
  {
    /// @brief Name of the profile (e.g., "temperature", "air_density", "ozone")
    std::string name{};

    /// @brief Units of the profile values (e.g., "K", "molecules/cm^3", "Dobson")
    std::string units{};

    /// @brief Number of cells in the profile
    /// @note Should match the associated grid's n_cells
    std::size_t n_cells{ 0 };

    /// @brief Scale height for exponential extrapolation (meters)
    ///
    /// Used when extrapolating profile values above the top of the grid.
    /// Typical value is ~8000 m for atmospheric scale height.
    double scale_height{ 8.0e3 };

    /// @brief Check equality between two ProfileSpec objects
    /// @param other The other ProfileSpec to compare
    /// @return True if name, units, and n_cells are all equal
    bool operator==(const ProfileSpec& other) const
    {
      return name == other.name && units == other.units && n_cells == other.n_cells;
    }

    /// @brief Check inequality between two ProfileSpec objects
    /// @param other The other ProfileSpec to compare
    /// @return True if any of name, units, or n_cells differ
    bool operator!=(const ProfileSpec& other) const
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
