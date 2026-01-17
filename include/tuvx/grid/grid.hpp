#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

#include <tuvx/grid/grid_spec.hpp>
#include <tuvx/util/array.hpp>
#include <tuvx/util/internal_error.hpp>

namespace tuvx
{
  /// @brief A 1D grid representing discrete cells with edges
  ///
  /// Grid represents a 1-dimensional discretization used for wavelength,
  /// altitude, time, or other coordinate systems. It stores cell edges
  /// and computes midpoints and cell widths (deltas).
  ///
  /// For a grid with n_cells cells, there are:
  /// - n_cells + 1 edges
  /// - n_cells midpoints
  /// - n_cells deltas (cell widths)
  class Grid
  {
   public:
    /// @brief Default constructor - creates an empty grid
    Grid() = default;

    /// @brief Construct a grid from edge values
    /// @param spec Grid specification (name, units, n_cells)
    /// @param edges Vector of edge values (must have n_cells + 1 elements)
    /// @throws TuvxInternalException if edges.size() != n_cells + 1
    Grid(GridSpec spec, std::vector<double> edges)
        : spec_(std::move(spec)),
          edges_(std::move(edges))
    {
      if (edges_.size() != spec_.n_cells + 1)
      {
        TUVX_INTERNAL_ERROR("Grid edges size must equal n_cells + 1");
      }
      ComputeMidpointsAndDeltas();
    }

    /// @brief Create an equally spaced grid
    /// @param spec Grid specification
    /// @param lower Lower bound (first edge)
    /// @param upper Upper bound (last edge)
    /// @return Grid with uniformly spaced edges
    static Grid EquallySpaced(GridSpec spec, double lower, double upper)
    {
      auto edges = array::Linspace(lower, upper, spec.n_cells + 1);
      return Grid(std::move(spec), std::move(edges));
    }

    /// @brief Create a logarithmically spaced grid
    /// @param spec Grid specification
    /// @param lower Lower bound (first edge, must be positive)
    /// @param upper Upper bound (last edge, must be positive)
    /// @return Grid with logarithmically spaced edges
    /// @throws TuvxInternalException if lower or upper are not positive
    static Grid LogarithmicallySpaced(GridSpec spec, double lower, double upper)
    {
      if (lower <= 0 || upper <= 0)
      {
        TUVX_INTERNAL_ERROR("Logarithmic grid requires positive bounds");
      }
      auto edges = array::Logspace(lower, upper, spec.n_cells + 1);
      return Grid(std::move(spec), std::move(edges));
    }

    /// @brief Get the grid specification
    /// @return Const reference to the grid spec
    const GridSpec& Spec() const
    {
      return spec_;
    }

    /// @brief Get the grid name
    /// @return Grid name from specification
    const std::string& Name() const
    {
      return spec_.name;
    }

    /// @brief Get the grid units
    /// @return Grid units from specification
    const std::string& Units() const
    {
      return spec_.units;
    }

    /// @brief Get the number of cells
    /// @return Number of cells in the grid
    std::size_t NumberOfCells() const
    {
      return spec_.n_cells;
    }

    /// @brief Get the cell edge values
    /// @return Span of edge values (n_cells + 1 elements)
    std::span<const double> Edges() const
    {
      return edges_;
    }

    /// @brief Get the cell midpoint values
    /// @return Span of midpoint values (n_cells elements)
    std::span<const double> Midpoints() const
    {
      return midpoints_;
    }

    /// @brief Get the cell widths (deltas)
    /// @return Span of delta values (n_cells elements)
    std::span<const double> Deltas() const
    {
      return deltas_;
    }

    /// @brief Get the lower bound of the grid
    /// @return First edge value
    double LowerBound() const
    {
      return edges_.front();
    }

    /// @brief Get the upper bound of the grid
    /// @return Last edge value
    double UpperBound() const
    {
      return edges_.back();
    }

    /// @brief Find the cell index containing a value
    /// @param value Value to locate
    /// @return Cell index (0 to n_cells-1) or nullopt if out of range
    ///
    /// Returns the index of the cell whose edges bracket the given value.
    /// The cell at index i spans [edges[i], edges[i+1]).
    /// For ascending grids, values at the upper boundary return the last cell.
    /// For descending grids, values at the upper boundary (lower numerical value)
    /// return the last cell.
    std::optional<std::size_t> FindCell(double value) const
    {
      if (spec_.n_cells == 0)
      {
        return std::nullopt;
      }

      bool ascending = edges_.front() <= edges_.back();

      if (ascending)
      {
        if (value < edges_.front() || value > edges_.back())
        {
          return std::nullopt;
        }
        // Binary search for the cell
        auto it = std::upper_bound(edges_.begin(), edges_.end(), value);
        if (it == edges_.begin())
        {
          return 0;
        }
        std::size_t idx = static_cast<std::size_t>(std::distance(edges_.begin(), it)) - 1;
        // Clamp to last cell for values at upper boundary
        if (idx >= spec_.n_cells)
        {
          idx = spec_.n_cells - 1;
        }
        return idx;
      }
      else
      {
        // Descending grid
        if (value > edges_.front() || value < edges_.back())
        {
          return std::nullopt;
        }
        // Binary search with reversed comparison
        auto it = std::upper_bound(edges_.begin(), edges_.end(), value, std::greater<double>{});
        if (it == edges_.begin())
        {
          return 0;
        }
        std::size_t idx = static_cast<std::size_t>(std::distance(edges_.begin(), it)) - 1;
        if (idx >= spec_.n_cells)
        {
          idx = spec_.n_cells - 1;
        }
        return idx;
      }
    }

   private:
    GridSpec spec_;
    std::vector<double> edges_;
    std::vector<double> midpoints_;
    std::vector<double> deltas_;

    /// @brief Compute midpoints and deltas from edges
    void ComputeMidpointsAndDeltas()
    {
      midpoints_.reserve(spec_.n_cells);
      deltas_.reserve(spec_.n_cells);

      for (std::size_t i = 0; i < spec_.n_cells; ++i)
      {
        midpoints_.push_back((edges_[i] + edges_[i + 1]) / 2.0);
        deltas_.push_back(edges_[i + 1] - edges_[i]);
      }
    }
  };

}  // namespace tuvx
