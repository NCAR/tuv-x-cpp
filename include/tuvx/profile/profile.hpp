#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

#include <tuvx/grid/grid.hpp>
#include <tuvx/profile/profile_spec.hpp>
#include <tuvx/util/internal_error.hpp>

namespace tuvx
{
  /// @brief Atmospheric profile data on a 1D grid
  ///
  /// Profile represents atmospheric data (temperature, density, mixing ratio, etc.)
  /// defined on a vertical grid. It stores both cell-center (midpoint) values
  /// and cell-edge values, and can compute derived quantities like layer densities
  /// and overhead column burdens.
  ///
  /// For a profile with n_cells cells:
  /// - mid_values: n_cells values at cell midpoints
  /// - edge_values: n_cells + 1 values at cell edges
  /// - layer_dens: n_cells integrated layer densities
  /// - burden_dens: n_cells + 1 overhead column amounts
  class Profile
  {
   public:
    /// @brief Construct a profile from midpoint values
    /// @param spec Profile specification (name, units, n_cells)
    /// @param mid_values Values at cell midpoints (n_cells elements)
    /// @throws TuvxInternalException if mid_values.size() != n_cells
    Profile(ProfileSpec spec, std::vector<double> mid_values)
        : spec_(std::move(spec)),
          mid_values_(std::move(mid_values))
    {
      if (mid_values_.size() != spec_.n_cells)
      {
        TUVX_INTERNAL_ERROR("Profile mid_values size must equal n_cells");
      }
      ComputeEdgeValuesFromMidpoints();
    }

    /// @brief Construct a profile from edge values
    /// @param spec Profile specification (name, units, n_cells)
    /// @param edge_values Values at cell edges (n_cells + 1 elements)
    /// @param from_edges Tag to distinguish from midpoint constructor
    /// @throws TuvxInternalException if edge_values.size() != n_cells + 1
    struct FromEdgesTag
    {
    };
    static constexpr FromEdgesTag FromEdges{};

    Profile(ProfileSpec spec, std::vector<double> edge_values, FromEdgesTag)
        : spec_(std::move(spec)),
          edge_values_(std::move(edge_values))
    {
      if (edge_values_.size() != spec_.n_cells + 1)
      {
        TUVX_INTERNAL_ERROR("Profile edge_values size must equal n_cells + 1");
      }
      ComputeMidValuesFromEdges();
    }

    /// @brief Get the profile specification
    /// @return Const reference to the profile spec
    const ProfileSpec& Spec() const
    {
      return spec_;
    }

    /// @brief Get the profile name
    /// @return Profile name from specification
    const std::string& Name() const
    {
      return spec_.name;
    }

    /// @brief Get the profile units
    /// @return Profile units from specification
    const std::string& Units() const
    {
      return spec_.units;
    }

    /// @brief Get the number of cells
    /// @return Number of cells in the profile
    std::size_t NumberOfCells() const
    {
      return spec_.n_cells;
    }

    /// @brief Get the scale height for extrapolation
    /// @return Scale height in meters
    double ScaleHeight() const
    {
      return spec_.scale_height;
    }

    /// @brief Get the cell midpoint values
    /// @return Span of midpoint values (n_cells elements)
    std::span<const double> MidValues() const
    {
      return mid_values_;
    }

    /// @brief Get the cell edge values
    /// @return Span of edge values (n_cells + 1 elements)
    std::span<const double> EdgeValues() const
    {
      return edge_values_;
    }

    /// @brief Get the layer density values
    /// @return Span of layer density values (n_cells elements)
    /// @note Call CalculateLayerDensities() first if not yet computed
    std::span<const double> LayerDensities() const
    {
      return layer_dens_;
    }

    /// @brief Get the overhead burden values
    /// @return Span of burden values (n_cells + 1 elements)
    /// @note Call CalculateBurden() first if not yet computed
    std::span<const double> BurdenDensities() const
    {
      return burden_dens_;
    }

    /// @brief Check if layer densities have been computed
    /// @return True if layer densities are available
    bool HasLayerDensities() const
    {
      return !layer_dens_.empty();
    }

    /// @brief Check if burden densities have been computed
    /// @return True if burden densities are available
    bool HasBurdenDensities() const
    {
      return !burden_dens_.empty();
    }

    /// @brief Calculate layer-integrated densities
    /// @param grid The grid defining layer boundaries
    ///
    /// Computes the integrated quantity within each grid cell:
    /// layer_dens[i] = mid_values[i] * delta[i]
    ///
    /// For example, if mid_values is number density (molecules/cm^3)
    /// and grid deltas are in cm, layer_dens will be column density
    /// (molecules/cm^2) for each layer.
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
    ///
    /// Computes the total column amount above each grid edge,
    /// integrating from the top of the atmosphere downward.
    ///
    /// burden_dens[n_cells] = 0 (nothing above top)
    /// burden_dens[i] = burden_dens[i+1] + layer_dens[i]
    ///
    /// This represents the slant column for overhead sun (SZA=0).
    void CalculateBurden(const Grid& grid)
    {
      if (!HasLayerDensities())
      {
        CalculateLayerDensities(grid);
      }

      burden_dens_.clear();
      burden_dens_.resize(spec_.n_cells + 1, 0.0);

      // Integrate from top down (assuming ascending altitude grid)
      // burden[n] = 0 (top of atmosphere)
      // burden[i] = burden[i+1] + layer[i]
      for (std::size_t i = spec_.n_cells; i > 0; --i)
      {
        burden_dens_[i - 1] = burden_dens_[i] + layer_dens_[i - 1];
      }
    }

    /// @brief Calculate extrapolated value above the grid
    /// @param altitude Altitude above the top of the grid (same units as grid)
    /// @param grid_top_altitude Top altitude of the grid
    /// @return Exponentially extrapolated value
    ///
    /// Uses scale height exponential decay:
    /// value(z) = value_top * exp(-(z - z_top) / scale_height)
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

   private:
    ProfileSpec spec_;
    std::vector<double> mid_values_;
    std::vector<double> edge_values_;
    std::vector<double> layer_dens_;
    std::vector<double> burden_dens_;

    /// @brief Compute edge values by interpolating/extrapolating midpoints
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

      // Extrapolate first edge
      edge_values_.push_back(1.5 * mid_values_[0] - 0.5 * mid_values_[1]);

      // Interpolate internal edges
      for (std::size_t i = 0; i < spec_.n_cells - 1; ++i)
      {
        edge_values_.push_back((mid_values_[i] + mid_values_[i + 1]) / 2.0);
      }

      // Extrapolate last edge
      edge_values_.push_back(1.5 * mid_values_[spec_.n_cells - 1] - 0.5 * mid_values_[spec_.n_cells - 2]);
    }

    /// @brief Compute midpoint values by averaging edge values
    void ComputeMidValuesFromEdges()
    {
      mid_values_.clear();
      mid_values_.reserve(spec_.n_cells);

      for (std::size_t i = 0; i < spec_.n_cells; ++i)
      {
        mid_values_.push_back((edge_values_[i] + edge_values_[i + 1]) / 2.0);
      }
    }
  };

}  // namespace tuvx
