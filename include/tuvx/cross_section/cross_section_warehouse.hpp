#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <tuvx/cross_section/cross_section.hpp>

namespace tuvx
{
  /// @brief Handle for fast cross-section lookup in warehouse
  ///
  /// CrossSectionHandle provides O(1) access to cross-sections in the warehouse
  /// after initial name-based lookup. Handles are stable for the lifetime of
  /// the warehouse.
  class CrossSectionHandle
  {
   public:
    /// @brief Default constructor creates an invalid handle
    CrossSectionHandle() = default;

    /// @brief Check if this handle is valid
    bool IsValid() const
    {
      return valid_;
    }

    /// @brief Get the index for direct access
    std::size_t Index() const
    {
      return index_;
    }

   private:
    friend class CrossSectionWarehouse;

    explicit CrossSectionHandle(std::size_t index) : index_(index), valid_(true)
    {
    }

    std::size_t index_{ 0 };
    bool valid_{ false };
  };

  /// @brief Collection manager for cross-sections
  ///
  /// CrossSectionWarehouse stores cross-sections and provides both name-based
  /// and handle-based access. Cross-sections are stored by unique_ptr and
  /// owned by the warehouse.
  class CrossSectionWarehouse
  {
   public:
    CrossSectionWarehouse() = default;

    // Prevent copying (owns unique_ptrs)
    CrossSectionWarehouse(const CrossSectionWarehouse&) = delete;
    CrossSectionWarehouse& operator=(const CrossSectionWarehouse&) = delete;

    // Allow moving
    CrossSectionWarehouse(CrossSectionWarehouse&&) = default;
    CrossSectionWarehouse& operator=(CrossSectionWarehouse&&) = default;

    /// @brief Add a cross-section to the warehouse
    /// @param cross_section The cross-section to add (takes ownership)
    /// @return Handle for fast subsequent access
    /// @throws std::runtime_error if a cross-section with the same name exists
    CrossSectionHandle Add(std::unique_ptr<CrossSection> cross_section)
    {
      if (!cross_section)
      {
        throw std::runtime_error("Cannot add null cross-section to warehouse");
      }

      const std::string& name = cross_section->Name();
      if (name_to_index_.count(name) > 0)
      {
        throw std::runtime_error("Cross-section '" + name + "' already exists in warehouse");
      }

      std::size_t index = cross_sections_.size();
      cross_sections_.push_back(std::move(cross_section));
      name_to_index_[name] = index;

      return CrossSectionHandle(index);
    }

    /// @brief Get cross-section by name
    /// @param name The cross-section name
    /// @return Reference to the cross-section
    /// @throws std::out_of_range if not found
    const CrossSection& Get(const std::string& name) const
    {
      auto it = name_to_index_.find(name);
      if (it == name_to_index_.end())
      {
        throw std::out_of_range("Cross-section '" + name + "' not found in warehouse");
      }
      return *cross_sections_[it->second];
    }

    /// @brief Get cross-section by handle (O(1) access)
    /// @param handle A valid handle from Add()
    /// @return Reference to the cross-section
    /// @throws std::out_of_range if handle is invalid
    const CrossSection& Get(CrossSectionHandle handle) const
    {
      if (!handle.IsValid() || handle.Index() >= cross_sections_.size())
      {
        throw std::out_of_range("Invalid cross-section handle");
      }
      return *cross_sections_[handle.Index()];
    }

    /// @brief Get handle for a cross-section by name
    /// @param name The cross-section name
    /// @return Handle for fast access
    /// @throws std::out_of_range if not found
    CrossSectionHandle GetHandle(const std::string& name) const
    {
      auto it = name_to_index_.find(name);
      if (it == name_to_index_.end())
      {
        throw std::out_of_range("Cross-section '" + name + "' not found in warehouse");
      }
      return CrossSectionHandle(it->second);
    }

    /// @brief Check if a cross-section exists
    /// @param name The cross-section name
    /// @return True if cross-section exists
    bool Exists(const std::string& name) const
    {
      return name_to_index_.count(name) > 0;
    }

    /// @brief Get all cross-section names
    /// @return Vector of names in insertion order
    std::vector<std::string> Names() const
    {
      std::vector<std::string> names;
      names.reserve(cross_sections_.size());
      for (const auto& xs : cross_sections_)
      {
        names.push_back(xs->Name());
      }
      return names;
    }

    /// @brief Get the number of cross-sections
    std::size_t Size() const
    {
      return cross_sections_.size();
    }

    /// @brief Check if warehouse is empty
    bool Empty() const
    {
      return cross_sections_.empty();
    }

    /// @brief Clear all cross-sections
    void Clear()
    {
      cross_sections_.clear();
      name_to_index_.clear();
    }

   private:
    std::vector<std::unique_ptr<CrossSection>> cross_sections_;
    std::unordered_map<std::string, std::size_t> name_to_index_;
  };

}  // namespace tuvx
