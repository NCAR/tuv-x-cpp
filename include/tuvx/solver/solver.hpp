#pragma once

#include <memory>
#include <string>

#include <tuvx/radiation_field/radiation_field.hpp>
#include <tuvx/radiator/radiator_state.hpp>
#include <tuvx/spherical_geometry/spherical_geometry.hpp>
#include <tuvx/surface/surface_albedo.hpp>

namespace tuvx
{
  /// @brief Input parameters for radiative transfer calculation
  struct SolverInput
  {
    /// Combined optical properties from all radiators
    const RadiatorState* radiator_state{ nullptr };

    /// Spherical geometry (slant paths)
    const SphericalGeometry::SlantPathResult* geometry{ nullptr };

    /// Surface albedo values [n_wavelengths]
    const std::vector<double>* surface_albedo{ nullptr };

    /// Extraterrestrial flux [n_wavelengths] (photons/cm^2/s)
    const std::vector<double>* extraterrestrial_flux{ nullptr };

    /// Solar zenith angle [degrees]
    double solar_zenith_angle{ 0.0 };

    /// Cosine of solar zenith angle
    double mu0() const
    {
      return std::cos(solar_zenith_angle * constants::kDegreesToRadians);
    }
  };

  /// @brief Abstract base class for radiative transfer solvers
  ///
  /// Solvers compute the radiation field (direct and diffuse irradiance,
  /// actinic flux) given the atmospheric optical properties and boundary
  /// conditions.
  class Solver
  {
   public:
    virtual ~Solver() = default;

    /// @brief Get solver name
    virtual std::string Name() const = 0;

    /// @brief Clone this solver
    virtual std::unique_ptr<Solver> Clone() const = 0;

    /// @brief Solve radiative transfer equation
    /// @param input Input parameters (optical properties, geometry, boundary conditions)
    /// @return Computed radiation field at all levels and wavelengths
    virtual RadiationField Solve(const SolverInput& input) const = 0;

    /// @brief Check if solver can handle given solar zenith angle
    /// @param sza Solar zenith angle [degrees]
    /// @return True if solver can compute for this SZA
    virtual bool CanHandle(double sza) const
    {
      // Default: can handle any SZA where sun is above horizon
      return sza < 90.0;
    }

   protected:
    Solver() = default;
    Solver(const Solver&) = default;
    Solver& operator=(const Solver&) = default;
  };

}  // namespace tuvx
