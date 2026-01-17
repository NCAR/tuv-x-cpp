#pragma once

#include <memory>
#include <string>

#include <tuvx/grid/grid_warehouse.hpp>
#include <tuvx/profile/profile_warehouse.hpp>
#include <tuvx/radiator/radiator_state.hpp>

namespace tuvx
{
  /// @brief Base class for atmospheric radiators
  ///
  /// A Radiator represents an atmospheric constituent that affects radiative
  /// transfer through absorption, emission, or scattering. Radiators compute
  /// their optical properties (optical depth, single scattering albedo,
  /// asymmetry factor) based on the current atmospheric state.
  ///
  /// Derived classes implement specific radiator types:
  /// - Molecular absorbers (O3, NO2, etc.) - computed from cross-sections
  /// - Rayleigh scattering - molecular scattering
  /// - Aerosols - particle scattering and absorption
  /// - Clouds - water/ice particle scattering
  ///
  /// The radiator's optical state is updated when atmospheric conditions
  /// change (e.g., new temperature/density profiles).
  class Radiator
  {
   public:
    virtual ~Radiator() = default;

    /// @brief Clone this radiator (for polymorphic copy)
    /// @return A unique_ptr to a copy of this radiator
    virtual std::unique_ptr<Radiator> Clone() const = 0;

    /// @brief Update optical state for current atmospheric conditions
    /// @param grids Available grids (wavelength, altitude)
    /// @param profiles Atmospheric profiles (density, temperature, etc.)
    ///
    /// This method recalculates the radiator's optical properties based
    /// on the current atmospheric state. It should be called whenever
    /// the atmospheric conditions change.
    virtual void UpdateState(const GridWarehouse& grids, const ProfileWarehouse& profiles) = 0;

    /// @brief Get the current optical state
    /// @return Reference to the radiator's optical properties
    const RadiatorState& State() const
    {
      return state_;
    }

    /// @brief Get the name of this radiator
    /// @return The radiator name (e.g., "O3", "aerosol", "rayleigh")
    const std::string& Name() const
    {
      return name_;
    }

    /// @brief Check if the state has been computed
    /// @return True if UpdateState() has been called
    bool HasState() const
    {
      return !state_.Empty();
    }

   protected:
    std::string name_{};
    RadiatorState state_{};
  };

}  // namespace tuvx
