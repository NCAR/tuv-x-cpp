#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

#include <tuvx/grid/grid.hpp>
#include <tuvx/profile/profile.hpp>
#include <tuvx/profile/profile_spec.hpp>
#include <tuvx/util/internal_error.hpp>

namespace tuvx
{
  /// @brief A mutable atmospheric profile that can be updated by host applications
  ///
  /// MutableProfile extends the Profile concept to allow values to be
  /// updated after construction. This is useful when profile data needs
  /// to change during runtime, such as temperature updates during a simulation.
  ///
  /// When values are modified, derived quantities (edge values, layer densities,
  /// burden) must be explicitly recalculated by calling Update().
  class MutableProfile
  {
   public:
    /// @brief Construct a mutable profile from midpoint values
    /// @param spec Profile specification (name, units, n_cells)
    /// @param mid_values Values at cell midpoints (n_cells elements)
    /// @throws TuvxInternalException if mid_values.size() != n_cells
    MutableProfile(ProfileSpec spec, std::vector<double> mid_values)
        : spec_(std::move(spec)),
          mid_values_(std::move(mid_values))
    {
      if (mid_values_.size() != spec_.n_cells)
      {
        TUVX_INTERNAL_ERROR("MutableProfile mid_values size must equal n_cells");
      }
      ComputeEdgeValuesFromMidpoints();
    }

    /// @brief Construct a mutable profile from edge values
    /// @param spec Profile specification
    /// @param edge_values Values at cell edges (n_cells + 1 elements)
    /// @param from_edges Tag to distinguish from midpoint constructor
    MutableProfile(ProfileSpec spec, std::vector<double> edge_values, Profile::FromEdgesTag)
        : spec_(std::move(spec)),
          edge_values_(std::move(edge_values))
    {
      if (edge_values_.size() != spec_.n_cells + 1)
      {
        TUVX_INTERNAL_ERROR("MutableProfile edge_values size must equal n_cells + 1");
      }
      ComputeMidValuesFromEdges();
    }

    /// @brief Construct a mutable profile from an immutable Profile
    /// @param profile Source profile to copy
    explicit MutableProfile(const Profile& profile)
        : spec_(profile.Spec()),
          mid_values_(profile.MidValues().begin(), profile.MidValues().end()),
          edge_values_(profile.EdgeValues().begin(), profile.EdgeValues().end())
    {
      // Copy layer/burden densities if available
      if (profile.HasLayerDensities())
      {
        layer_dens_.assign(profile.LayerDensities().begin(), profile.LayerDensities().end());
      }
      if (profile.HasBurdenDensities())
      {
        burden_dens_.assign(profile.BurdenDensities().begin(), profile.BurdenDensities().end());
      }
    }

    /// @brief Get the profile specification
    const ProfileSpec& Spec() const
    {
      return spec_;
    }

    /// @brief Get the profile name
    const std::string& Name() const
    {
      return spec_.name;
    }

    /// @brief Get the profile units
    const std::string& Units() const
    {
      return spec_.units;
    }

    /// @brief Get the number of cells
    std::size_t NumberOfCells() const
    {
      return spec_.n_cells;
    }

    /// @brief Get the scale height for extrapolation
    double ScaleHeight() const
    {
      return spec_.scale_height;
    }

    /// @brief Get the cell midpoint values (read-only)
    std::span<const double> MidValues() const
    {
      return mid_values_;
    }

    /// @brief Get mutable access to midpoint values
    /// @note Call Update() after modifying to recompute derived values
    std::span<double> MutableMidValues()
    {
      return mid_values_;
    }

    /// @brief Get the cell edge values
    std::span<const double> EdgeValues() const
    {
      return edge_values_;
    }

    /// @brief Get mutable access to edge values
    /// @note Call Update(FromEdges) after modifying to recompute mid values
    std::span<double> MutableEdgeValues()
    {
      return edge_values_;
    }

    /// @brief Get the layer density values
    std::span<const double> LayerDensities() const
    {
      return layer_dens_;
    }

    /// @brief Get the overhead burden values
    std::span<const double> BurdenDensities() const
    {
      return burden_dens_;
    }

    /// @brief Check if layer densities have been computed
    bool HasLayerDensities() const
    {
      return !layer_dens_.empty();
    }

    /// @brief Check if burden densities have been computed
    bool HasBurdenDensities() const
    {
      return !burden_dens_.empty();
    }

    /// @brief Set all midpoint values at once
    /// @param new_values New midpoint values
    /// @throws TuvxInternalException if size mismatch
    void SetMidValues(std::span<const double> new_values)
    {
      if (new_values.size() != mid_values_.size())
      {
        TUVX_INTERNAL_ERROR("New mid values must have same size as existing");
      }
      std::copy(new_values.begin(), new_values.end(), mid_values_.begin());
      ComputeEdgeValuesFromMidpoints();
      InvalidateDerivedQuantities();
    }

    /// @brief Set a single midpoint value
    /// @param index Cell index
    /// @param value New value
    /// @note Call Update() after all modifications
    void SetMidValue(std::size_t index, double value)
    {
      if (index >= mid_values_.size())
      {
        TUVX_INTERNAL_ERROR("Mid value index out of range");
      }
      mid_values_[index] = value;
    }

    /// @brief Set all edge values at once
    /// @param new_values New edge values
    /// @throws TuvxInternalException if size mismatch
    void SetEdgeValues(std::span<const double> new_values)
    {
      if (new_values.size() != edge_values_.size())
      {
        TUVX_INTERNAL_ERROR("New edge values must have same size as existing");
      }
      std::copy(new_values.begin(), new_values.end(), edge_values_.begin());
      ComputeMidValuesFromEdges();
      InvalidateDerivedQuantities();
    }

    /// @brief Update derived values from midpoint modifications
    ///
    /// Call this after modifying midpoint values through MutableMidValues()
    /// or SetMidValue() to recompute edge values.
    void Update()
    {
      ComputeEdgeValuesFromMidpoints();
      InvalidateDerivedQuantities();
    }

    /// @brief Update derived values from edge modifications
    ///
    /// Call this after modifying edge values through MutableEdgeValues()
    /// to recompute midpoint values.
    void UpdateFromEdges()
    {
      ComputeMidValuesFromEdges();
      InvalidateDerivedQuantities();
    }

    /// @brief Calculate layer-integrated densities
    /// @param grid The grid defining layer boundaries
    void CalculateLayerDensities(const Grid& grid)
    {
      if (grid.NumberOfCells() != spec_.n_cells)
      {
        TUVX_INTERNAL_ERROR("Grid size does not match profile size");
      }

      layer_dens_.clear();
      layer_dens_.reserve(spec_.n_cells);

      auto deltas = grid.Deltas();
      for (std::size_t i = 0; i < spec_.n_cells; ++i)
      {
        layer_dens_.push_back(mid_values_[i] * deltas[i]);
      }
    }

    /// @brief Calculate overhead column burden
    /// @param grid The grid defining layer boundaries
    void CalculateBurden(const Grid& grid)
    {
      if (!HasLayerDensities())
      {
        CalculateLayerDensities(grid);
      }

      burden_dens_.clear();
      burden_dens_.resize(spec_.n_cells + 1, 0.0);

      for (std::size_t i = spec_.n_cells; i > 0; --i)
      {
        burden_dens_[i - 1] = burden_dens_[i] + layer_dens_[i - 1];
      }
    }

    /// @brief Calculate extrapolated value above the grid
    /// @param altitude Altitude above the top of the grid
    /// @param grid_top_altitude Top altitude of the grid
    /// @return Exponentially extrapolated value
    double ExtrapolateAbove(double altitude, double grid_top_altitude) const
    {
      if (mid_values_.empty())
      {
        return 0.0;
      }
      double value_top = mid_values_.back();
      double dz = altitude - grid_top_altitude;
      if (dz <= 0)
      {
        return value_top;
      }
      return value_top * std::exp(-dz / spec_.scale_height);
    }

    /// @brief Convert to an immutable Profile
    /// @return A new Profile with copy of current state
    Profile ToProfile() const
    {
      return Profile(spec_, mid_values_);
    }

   private:
    ProfileSpec spec_;
    std::vector<double> mid_values_;
    std::vector<double> edge_values_;
    std::vector<double> layer_dens_;
    std::vector<double> burden_dens_;

    void ComputeEdgeValuesFromMidpoints()
    {
      edge_values_.clear();
      edge_values_.reserve(spec_.n_cells + 1);

      if (spec_.n_cells == 0)
      {
        edge_values_.push_back(0.0);
        return;
      }

      if (spec_.n_cells == 1)
      {
        edge_values_.push_back(mid_values_[0]);
        edge_values_.push_back(mid_values_[0]);
        return;
      }

      edge_values_.push_back(1.5 * mid_values_[0] - 0.5 * mid_values_[1]);
      for (std::size_t i = 0; i < spec_.n_cells - 1; ++i)
      {
        edge_values_.push_back((mid_values_[i] + mid_values_[i + 1]) / 2.0);
      }
      edge_values_.push_back(1.5 * mid_values_[spec_.n_cells - 1] - 0.5 * mid_values_[spec_.n_cells - 2]);
    }

    void ComputeMidValuesFromEdges()
    {
      mid_values_.clear();
      mid_values_.reserve(spec_.n_cells);

      for (std::size_t i = 0; i < spec_.n_cells; ++i)
      {
        mid_values_.push_back((edge_values_[i] + edge_values_[i + 1]) / 2.0);
      }
    }

    void InvalidateDerivedQuantities()
    {
      layer_dens_.clear();
      burden_dens_.clear();
    }
  };

}  // namespace tuvx
