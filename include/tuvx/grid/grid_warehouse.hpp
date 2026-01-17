#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include <tuvx/grid/grid.hpp>
#include <tuvx/grid/grid_spec.hpp>
#include <tuvx/util/internal_error.hpp>

namespace tuvx
{
  /// @brief Handle for accessing a grid in a warehouse
  ///
  /// GridHandle provides O(1) access to grids stored in a GridWarehouse.
  /// Handles are returned when adding grids and can be used for efficient
  /// repeated access without string-based lookup.
  class GridHandle
  {
   public:
    /// @brief Default constructor creates an invalid handle
    GridHandle() = default;

    /// @brief Check if handle is valid
    /// @return True if handle points to a valid grid
    bool IsValid() const
    {
      return valid_;
    }

    /// @brief Get the index of the referenced grid
    /// @return Grid index in the warehouse
    std::size_t Index() const
    {
      return index_;
    }

    /// @brief Equality comparison
    bool operator==(const GridHandle& other) const
    {
      return valid_ == other.valid_ && index_ == other.index_;
    }

    /// @brief Inequality comparison
    bool operator!=(const GridHandle& other) const
    {
      return !(*this == other);
    }

   private:
    friend class GridWarehouse;

    /// @brief Construct a valid handle with index
    explicit GridHandle(std::size_t index)
        : index_(index),
          valid_(true)
    {
    }

    std::size_t index_{ 0 };
    bool valid_{ false };
  };

  /// @brief Collection manager for Grid objects
  ///
  /// GridWarehouse stores and manages a collection of Grid objects,
  /// providing both name/units-based lookup and handle-based access.
  /// Each grid is uniquely identified by the combination of its name
  /// and units (e.g., "wavelength|nm").
  class GridWarehouse
  {
   public:
    /// @brief Add a grid to the warehouse
    /// @param grid Grid to add
    /// @return Handle for accessing the grid
    /// @throws TuvxInternalException if a grid with the same name/units already exists
    GridHandle Add(Grid grid)
    {
      std::string key = grid.Spec().Key();
      if (key_to_index_.find(key) != key_to_index_.end())
      {
        TUVX_INTERNAL_ERROR("Grid with this name and units already exists in warehouse");
      }

      std::size_t index = grids_.size();
      grids_.push_back(std::move(grid));
      key_to_index_[key] = index;

      return GridHandle(index);
    }

    /// @brief Check if a grid exists in the warehouse
    /// @param name Grid name
    /// @param units Grid units
    /// @return True if a grid with the given name/units exists
    bool Exists(const std::string& name, const std::string& units) const
    {
      std::string key = name + "|" + units;
      return key_to_index_.find(key) != key_to_index_.end();
    }

    /// @brief Get a grid by name and units
    /// @param name Grid name
    /// @param units Grid units
    /// @return Const reference to the grid
    /// @throws TuvxInternalException if grid not found
    const Grid& Get(const std::string& name, const std::string& units) const
    {
      std::string key = name + "|" + units;
      auto it = key_to_index_.find(key);
      if (it == key_to_index_.end())
      {
        TUVX_INTERNAL_ERROR("Grid not found in warehouse");
      }
      return grids_[it->second];
    }

    /// @brief Get a grid by handle
    /// @param handle Grid handle from Add()
    /// @return Const reference to the grid
    /// @throws TuvxInternalException if handle is invalid
    const Grid& Get(GridHandle handle) const
    {
      if (!handle.IsValid() || handle.Index() >= grids_.size())
      {
        TUVX_INTERNAL_ERROR("Invalid grid handle");
      }
      return grids_[handle.Index()];
    }

    /// @brief Get a handle for a grid by name and units
    /// @param name Grid name
    /// @param units Grid units
    /// @return Handle to the grid, or invalid handle if not found
    GridHandle GetHandle(const std::string& name, const std::string& units) const
    {
      std::string key = name + "|" + units;
      auto it = key_to_index_.find(key);
      if (it == key_to_index_.end())
      {
        return GridHandle{};  // Invalid handle
      }
      return GridHandle(it->second);
    }

    /// @brief Get the number of grids in the warehouse
    /// @return Grid count
    std::size_t Size() const
    {
      return grids_.size();
    }

    /// @brief Check if warehouse is empty
    /// @return True if no grids stored
    bool Empty() const
    {
      return grids_.empty();
    }

    /// @brief Get all grid names in the warehouse
    /// @return Vector of grid name|units keys
    std::vector<std::string> Keys() const
    {
      std::vector<std::string> result;
      result.reserve(grids_.size());
      for (const auto& grid : grids_)
      {
        result.push_back(grid.Spec().Key());
      }
      return result;
    }

    /// @brief Remove all grids from the warehouse
    void Clear()
    {
      grids_.clear();
      key_to_index_.clear();
    }

   private:
    std::vector<Grid> grids_;
    std::unordered_map<std::string, std::size_t> key_to_index_;
  };

}  // namespace tuvx
