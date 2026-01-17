#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include <tuvx/profile/profile.hpp>
#include <tuvx/profile/profile_spec.hpp>
#include <tuvx/util/internal_error.hpp>

namespace tuvx
{
  /// @brief Handle for accessing a profile in a warehouse
  ///
  /// ProfileHandle provides O(1) access to profiles stored in a ProfileWarehouse.
  /// Handles are returned when adding profiles and can be used for efficient
  /// repeated access without string-based lookup.
  class ProfileHandle
  {
   public:
    /// @brief Default constructor creates an invalid handle
    ProfileHandle() = default;

    /// @brief Check if handle is valid
    /// @return True if handle points to a valid profile
    bool IsValid() const
    {
      return valid_;
    }

    /// @brief Get the index of the referenced profile
    /// @return Profile index in the warehouse
    std::size_t Index() const
    {
      return index_;
    }

    /// @brief Equality comparison
    bool operator==(const ProfileHandle& other) const
    {
      return valid_ == other.valid_ && index_ == other.index_;
    }

    /// @brief Inequality comparison
    bool operator!=(const ProfileHandle& other) const
    {
      return !(*this == other);
    }

   private:
    friend class ProfileWarehouse;

    /// @brief Construct a valid handle with index
    explicit ProfileHandle(std::size_t index)
        : index_(index),
          valid_(true)
    {
    }

    std::size_t index_{ 0 };
    bool valid_{ false };
  };

  /// @brief Collection manager for Profile objects
  ///
  /// ProfileWarehouse stores and manages a collection of Profile objects,
  /// providing both name/units-based lookup and handle-based access.
  /// Each profile is uniquely identified by the combination of its name
  /// and units (e.g., "temperature|K").
  class ProfileWarehouse
  {
   public:
    /// @brief Add a profile to the warehouse
    /// @param profile Profile to add
    /// @return Handle for accessing the profile
    /// @throws TuvxInternalException if a profile with the same name/units already exists
    ProfileHandle Add(Profile profile)
    {
      std::string key = profile.Spec().Key();
      if (key_to_index_.find(key) != key_to_index_.end())
      {
        TUVX_INTERNAL_ERROR("Profile with this name and units already exists in warehouse");
      }

      std::size_t index = profiles_.size();
      profiles_.push_back(std::move(profile));
      key_to_index_[key] = index;

      return ProfileHandle(index);
    }

    /// @brief Check if a profile exists in the warehouse
    /// @param name Profile name
    /// @param units Profile units
    /// @return True if a profile with the given name/units exists
    bool Exists(const std::string& name, const std::string& units) const
    {
      std::string key = name + "|" + units;
      return key_to_index_.find(key) != key_to_index_.end();
    }

    /// @brief Get a profile by name and units
    /// @param name Profile name
    /// @param units Profile units
    /// @return Const reference to the profile
    /// @throws TuvxInternalException if profile not found
    const Profile& Get(const std::string& name, const std::string& units) const
    {
      std::string key = name + "|" + units;
      auto it = key_to_index_.find(key);
      if (it == key_to_index_.end())
      {
        TUVX_INTERNAL_ERROR("Profile not found in warehouse");
      }
      return profiles_[it->second];
    }

    /// @brief Get a profile by handle
    /// @param handle Profile handle from Add()
    /// @return Const reference to the profile
    /// @throws TuvxInternalException if handle is invalid
    const Profile& Get(ProfileHandle handle) const
    {
      if (!handle.IsValid() || handle.Index() >= profiles_.size())
      {
        TUVX_INTERNAL_ERROR("Invalid profile handle");
      }
      return profiles_[handle.Index()];
    }

    /// @brief Get a handle for a profile by name and units
    /// @param name Profile name
    /// @param units Profile units
    /// @return Handle to the profile, or invalid handle if not found
    ProfileHandle GetHandle(const std::string& name, const std::string& units) const
    {
      std::string key = name + "|" + units;
      auto it = key_to_index_.find(key);
      if (it == key_to_index_.end())
      {
        return ProfileHandle{};  // Invalid handle
      }
      return ProfileHandle(it->second);
    }

    /// @brief Get the number of profiles in the warehouse
    /// @return Profile count
    std::size_t Size() const
    {
      return profiles_.size();
    }

    /// @brief Check if warehouse is empty
    /// @return True if no profiles stored
    bool Empty() const
    {
      return profiles_.empty();
    }

    /// @brief Get all profile names in the warehouse
    /// @return Vector of profile name|units keys
    std::vector<std::string> Keys() const
    {
      std::vector<std::string> result;
      result.reserve(profiles_.size());
      for (const auto& profile : profiles_)
      {
        result.push_back(profile.Spec().Key());
      }
      return result;
    }

    /// @brief Remove all profiles from the warehouse
    void Clear()
    {
      profiles_.clear();
      key_to_index_.clear();
    }

   private:
    std::vector<Profile> profiles_;
    std::unordered_map<std::string, std::size_t> key_to_index_;
  };

}  // namespace tuvx
