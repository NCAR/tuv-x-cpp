#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <tuvx/radiator/radiator.hpp>
#include <tuvx/radiator/radiator_state.hpp>

namespace tuvx
{
  /// @brief Handle for fast radiator lookup in warehouse
  class RadiatorHandle
  {
   public:
    RadiatorHandle() = default;

    bool IsValid() const
    {
      return valid_;
    }

    std::size_t Index() const
    {
      return index_;
    }

   private:
    friend class RadiatorWarehouse;

    explicit RadiatorHandle(std::size_t index) : index_(index), valid_(true)
    {
    }

    std::size_t index_{ 0 };
    bool valid_{ false };
  };

  /// @brief Collection manager for radiators
  ///
  /// RadiatorWarehouse stores radiators and provides functionality to
  /// update all radiators simultaneously and compute combined atmospheric
  /// optical properties.
  class RadiatorWarehouse
  {
   public:
    RadiatorWarehouse() = default;

    // Prevent copying
    RadiatorWarehouse(const RadiatorWarehouse&) = delete;
    RadiatorWarehouse& operator=(const RadiatorWarehouse&) = delete;

    // Allow moving
    RadiatorWarehouse(RadiatorWarehouse&&) = default;
    RadiatorWarehouse& operator=(RadiatorWarehouse&&) = default;

    /// @brief Add a radiator to the warehouse
    /// @param radiator The radiator to add (takes ownership)
    /// @return Handle for fast subsequent access
    /// @throws std::runtime_error if radiator with same name exists
    RadiatorHandle Add(std::unique_ptr<Radiator> radiator)
    {
      if (!radiator)
      {
        throw std::runtime_error("Cannot add null radiator to warehouse");
      }

      const std::string& name = radiator->Name();
      if (name_to_index_.count(name) > 0)
      {
        throw std::runtime_error("Radiator '" + name + "' already exists in warehouse");
      }

      std::size_t index = radiators_.size();
      radiators_.push_back(std::move(radiator));
      name_to_index_[name] = index;

      return RadiatorHandle(index);
    }

    /// @brief Get radiator by name
    const Radiator& Get(const std::string& name) const
    {
      auto it = name_to_index_.find(name);
      if (it == name_to_index_.end())
      {
        throw std::out_of_range("Radiator '" + name + "' not found in warehouse");
      }
      return *radiators_[it->second];
    }

    /// @brief Get radiator by handle
    const Radiator& Get(RadiatorHandle handle) const
    {
      if (!handle.IsValid() || handle.Index() >= radiators_.size())
      {
        throw std::out_of_range("Invalid radiator handle");
      }
      return *radiators_[handle.Index()];
    }

    /// @brief Get mutable radiator by name (for updating state)
    Radiator& GetMutable(const std::string& name)
    {
      auto it = name_to_index_.find(name);
      if (it == name_to_index_.end())
      {
        throw std::out_of_range("Radiator '" + name + "' not found in warehouse");
      }
      return *radiators_[it->second];
    }

    /// @brief Get mutable radiator by handle
    Radiator& GetMutable(RadiatorHandle handle)
    {
      if (!handle.IsValid() || handle.Index() >= radiators_.size())
      {
        throw std::out_of_range("Invalid radiator handle");
      }
      return *radiators_[handle.Index()];
    }

    /// @brief Get handle for a radiator by name
    RadiatorHandle GetHandle(const std::string& name) const
    {
      auto it = name_to_index_.find(name);
      if (it == name_to_index_.end())
      {
        throw std::out_of_range("Radiator '" + name + "' not found in warehouse");
      }
      return RadiatorHandle(it->second);
    }

    /// @brief Check if radiator exists
    bool Exists(const std::string& name) const
    {
      return name_to_index_.count(name) > 0;
    }

    /// @brief Get all radiator names
    std::vector<std::string> Names() const
    {
      std::vector<std::string> names;
      names.reserve(radiators_.size());
      for (const auto& r : radiators_)
      {
        names.push_back(r->Name());
      }
      return names;
    }

    /// @brief Get number of radiators
    std::size_t Size() const
    {
      return radiators_.size();
    }

    /// @brief Check if warehouse is empty
    bool Empty() const
    {
      return radiators_.empty();
    }

    /// @brief Update all radiators with current atmospheric conditions
    /// @param grids Grid warehouse
    /// @param profiles Profile warehouse
    void UpdateAll(const GridWarehouse& grids, const ProfileWarehouse& profiles)
    {
      for (auto& radiator : radiators_)
      {
        radiator->UpdateState(grids, profiles);
      }
    }

    /// @brief Get combined atmospheric state from all radiators
    /// @return Combined optical properties
    ///
    /// Accumulates optical properties from all radiators following
    /// radiative transfer combination rules.
    RadiatorState CombinedState() const
    {
      RadiatorState combined;

      for (const auto& radiator : radiators_)
      {
        if (radiator->HasState())
        {
          combined.Accumulate(radiator->State());
        }
      }

      return combined;
    }

    /// @brief Clear all radiators
    void Clear()
    {
      radiators_.clear();
      name_to_index_.clear();
    }

   private:
    std::vector<std::unique_ptr<Radiator>> radiators_;
    std::unordered_map<std::string, std::size_t> name_to_index_;
  };

}  // namespace tuvx
