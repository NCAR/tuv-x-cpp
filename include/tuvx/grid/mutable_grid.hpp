#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include <tuvx/grid/grid.hpp>
#include <tuvx/grid/grid_spec.hpp>
#include <tuvx/util/internal_error.hpp>

namespace tuvx
{
  /// @brief A mutable 1D grid that can be updated by host applications
  ///
  /// MutableGrid extends the Grid concept to allow edge values to be
  /// updated after construction. This is useful when grids need to be
  /// modified during runtime, such as adaptive mesh refinement or
  /// time-varying coordinates.
  ///
  /// When edges are updated, midpoints and deltas are automatically
  /// recomputed.
  class MutableGrid
  {
   public:
    /// @brief Construct a mutable grid from edge values
    /// @param spec Grid specification (name, units, n_cells)
    /// @param edges Vector of edge values (must have n_cells + 1 elements)
    /// @throws TuvxInternalException if edges.size() != n_cells + 1
    MutableGrid(GridSpec spec, std::vector<double> edges)
        : spec_(std::move(spec)),
          edges_(std::move(edges))
    {
      if (edges_.size() != spec_.n_cells + 1)
      {
        TUVX_INTERNAL_ERROR("MutableGrid edges size must equal n_cells + 1");
      }
      ComputeMidpointsAndDeltas();
    }

    /// @brief Construct a mutable grid from an immutable Grid
    /// @param grid Source grid to copy
    explicit MutableGrid(const Grid& grid)
        : spec_(grid.Spec()),
          edges_(grid.Edges().begin(), grid.Edges().end())
    {
      ComputeMidpointsAndDeltas();
    }

    /// @brief Create an equally spaced mutable grid
    /// @param spec Grid specification
    /// @param lower Lower bound (first edge)
    /// @param upper Upper bound (last edge)
    /// @return MutableGrid with uniformly spaced edges
    static MutableGrid EquallySpaced(GridSpec spec, double lower, double upper)
    {
      auto edges = array::Linspace(lower, upper, spec.n_cells + 1);
      return MutableGrid(std::move(spec), std::move(edges));
    }

    /// @brief Create a logarithmically spaced mutable grid
    /// @param spec Grid specification
    /// @param lower Lower bound (first edge, must be positive)
    /// @param upper Upper bound (last edge, must be positive)
    /// @return MutableGrid with logarithmically spaced edges
    /// @throws TuvxInternalException if lower or upper are not positive
    static MutableGrid LogarithmicallySpaced(GridSpec spec, double lower, double upper)
    {
      if (lower <= 0 || upper <= 0)
      {
        TUVX_INTERNAL_ERROR("Logarithmic grid requires positive bounds");
      }
      auto edges = array::Logspace(lower, upper, spec.n_cells + 1);
      return MutableGrid(std::move(spec), std::move(edges));
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

    /// @brief Get the cell edge values (read-only)
    /// @return Span of edge values (n_cells + 1 elements)
    std::span<const double> Edges() const
    {
      return edges_;
    }

    /// @brief Get mutable access to edge values
    /// @return Span of mutable edge values
    /// @note Call Update() after modifying edges to recompute derived values
    std::span<double> MutableEdges()
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

    /// @brief Set all edge values at once
    /// @param new_edges New edge values (must have same size as current edges)
    /// @throws TuvxInternalException if new_edges.size() != n_cells + 1
    void SetEdges(std::span<const double> new_edges)
    {
      if (new_edges.size() != edges_.size())
      {
        TUVX_INTERNAL_ERROR("New edges must have same size as existing edges");
      }
      std::copy(new_edges.begin(), new_edges.end(), edges_.begin());
      ComputeMidpointsAndDeltas();
    }

    /// @brief Set a single edge value
    /// @param index Edge index to update
    /// @param value New value for the edge
    /// @throws TuvxInternalException if index is out of range
    /// @note Call Update() after all edge modifications to recompute derived values
    void SetEdge(std::size_t index, double value)
    {
      if (index >= edges_.size())
      {
        TUVX_INTERNAL_ERROR("Edge index out of range");
      }
      edges_[index] = value;
    }

    /// @brief Recompute midpoints and deltas after edge modifications
    ///
    /// Call this method after modifying edges through MutableEdges() or SetEdge()
    /// to update the derived midpoint and delta values.
    void Update()
    {
      ComputeMidpointsAndDeltas();
    }

    /// @brief Find the cell index containing a value
    /// @param value Value to locate
    /// @return Cell index (0 to n_cells-1) or nullopt if out of range
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
        auto it = std::upper_bound(edges_.begin(), edges_.end(), value);
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
      else
      {
        if (value > edges_.front() || value < edges_.back())
        {
          return std::nullopt;
        }
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

    /// @brief Convert to an immutable Grid
    /// @return A new Grid with a copy of current state
    Grid ToGrid() const
    {
      return Grid(spec_, edges_);
    }

   private:
    GridSpec spec_;
    std::vector<double> edges_;
    std::vector<double> midpoints_;
    std::vector<double> deltas_;

    void ComputeMidpointsAndDeltas()
    {
      midpoints_.clear();
      deltas_.clear();
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
