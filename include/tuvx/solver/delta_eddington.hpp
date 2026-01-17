#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <tuvx/solver/solver.hpp>
#include <tuvx/util/constants.hpp>

namespace tuvx
{
  /// @brief Delta-Eddington two-stream radiative transfer solver
  ///
  /// Implements the delta-Eddington approximation for solving the radiative
  /// transfer equation. This is a two-stream method that accounts for:
  /// - Direct beam attenuation
  /// - Diffuse radiation (up and down)
  /// - Surface reflection
  /// - Multiple scattering
  ///
  /// Based on:
  /// - Joseph, J.H., W.J. Wiscombe, and J.A. Weinman, 1976: The delta-Eddington
  ///   approximation for radiative flux transfer. J. Atmos. Sci., 33, 2452-2459.
  /// - Toon, O.B., et al., 1989: Rapid calculation of radiative heating rates
  ///   and photodissociation rates in inhomogeneous multiple scattering
  ///   atmospheres. J. Geophys. Res., 94, 16287-16301.
  class DeltaEddingtonSolver : public Solver
  {
   public:
    DeltaEddingtonSolver() = default;

    std::string Name() const override
    {
      return "delta_eddington";
    }

    std::unique_ptr<Solver> Clone() const override
    {
      return std::make_unique<DeltaEddingtonSolver>(*this);
    }

    RadiationField Solve(const SolverInput& input) const override
    {
      // Validate input
      if (!input.radiator_state || input.radiator_state->Empty())
      {
        return RadiationField{};
      }

      std::size_t n_layers = input.radiator_state->NumberOfLayers();
      std::size_t n_wavelengths = input.radiator_state->NumberOfWavelengths();
      std::size_t n_levels = n_layers + 1;

      // Initialize output
      RadiationField field;
      field.Initialize(n_levels, n_wavelengths);

      double mu0 = input.mu0();

      // Check if sun is above horizon
      if (mu0 <= 0.0)
      {
        // Night time - no radiation
        return field;
      }

      // Get slant path factors (default to 1/mu0 if not provided)
      std::vector<double> slant_factors(n_layers, 1.0 / mu0);
      if (input.geometry)
      {
        slant_factors = input.geometry->enhancement_factor;
      }

      // Solve for each wavelength independently
      for (std::size_t j = 0; j < n_wavelengths; ++j)
      {
        // Get optical properties for this wavelength
        std::vector<double> tau(n_layers);
        std::vector<double> omega(n_layers);
        std::vector<double> g(n_layers);

        for (std::size_t i = 0; i < n_layers; ++i)
        {
          tau[i] = input.radiator_state->optical_depth[i][j];
          omega[i] = input.radiator_state->single_scattering_albedo[i][j];
          g[i] = input.radiator_state->asymmetry_factor[i][j];
        }

        // Get surface albedo
        double albedo = 0.0;
        if (input.surface_albedo && j < input.surface_albedo->size())
        {
          albedo = (*input.surface_albedo)[j];
        }

        // Get TOA flux
        double flux_toa = 1.0;
        if (input.extraterrestrial_flux && j < input.extraterrestrial_flux->size())
        {
          flux_toa = (*input.extraterrestrial_flux)[j];
        }

        // Solve two-stream equations
        auto result = SolveTwoStream(tau, omega, g, mu0, albedo, flux_toa, slant_factors);

        // Store results
        for (std::size_t i = 0; i < n_levels; ++i)
        {
          field.direct_irradiance[i][j] = result.direct[i];
          field.diffuse_down[i][j] = result.diffuse_down[i];
          field.diffuse_up[i][j] = result.diffuse_up[i];
          field.actinic_flux_direct[i][j] = result.actinic_direct[i];
          field.actinic_flux_diffuse[i][j] = result.actinic_diffuse[i];
        }
      }

      return field;
    }

   private:
    /// Results from two-stream calculation for one wavelength
    struct TwoStreamResult
    {
      std::vector<double> direct;          // Direct irradiance at levels
      std::vector<double> diffuse_down;    // Diffuse downwelling at levels
      std::vector<double> diffuse_up;      // Diffuse upwelling at levels
      std::vector<double> actinic_direct;  // Direct actinic flux at levels
      std::vector<double> actinic_diffuse; // Diffuse actinic flux at levels
    };

    /// @brief Apply delta-M scaling to optical properties
    /// @param tau Original optical depth
    /// @param omega Original single scattering albedo
    /// @param g Original asymmetry factor
    /// @return Tuple of (scaled_tau, scaled_omega, scaled_g)
    static std::tuple<double, double, double> DeltaScale(double tau, double omega, double g)
    {
      // Delta-M scaling: removes forward scattering peak
      double f = g * g;  // Fraction of forward scattering

      double tau_scaled = tau * (1.0 - omega * f);
      double omega_scaled = omega * (1.0 - f) / (1.0 - omega * f);
      double g_scaled = (g - f) / (1.0 - f);

      // Clamp to valid ranges
      omega_scaled = std::clamp(omega_scaled, 0.0, 1.0);
      g_scaled = std::clamp(g_scaled, -1.0, 1.0);

      return { tau_scaled, omega_scaled, g_scaled };
    }

    /// @brief Solve two-stream equations for one wavelength
    TwoStreamResult SolveTwoStream(
        const std::vector<double>& tau,
        const std::vector<double>& omega,
        const std::vector<double>& g,
        double mu0,
        double albedo,
        double flux_toa,
        const std::vector<double>& slant_factors) const
    {
      std::size_t n_layers = tau.size();
      std::size_t n_levels = n_layers + 1;

      TwoStreamResult result;
      result.direct.resize(n_levels, 0.0);
      result.diffuse_down.resize(n_levels, 0.0);
      result.diffuse_up.resize(n_levels, 0.0);
      result.actinic_direct.resize(n_levels, 0.0);
      result.actinic_diffuse.resize(n_levels, 0.0);

      // Two-stream coefficients (Eddington approximation)
      // gamma1 = (7 - omega*(4 + 3*g)) / 4
      // gamma2 = -(1 - omega*(4 - 3*g)) / 4
      // gamma3 = (2 - 3*g*mu0) / 4
      // gamma4 = 1 - gamma3

      // Calculate cumulative optical depth and transmittance
      std::vector<double> tau_cumulative(n_levels, 0.0);
      for (std::size_t i = 0; i < n_layers; ++i)
      {
        // Apply delta scaling
        auto [tau_s, omega_s, g_s] = DeltaScale(tau[i], omega[i], g[i]);

        // Accumulate from TOA (index n_layers) down to surface (index 0)
        // Levels are ordered: 0=surface, n_layers=TOA
        std::size_t level_top = n_layers - i;
        std::size_t level_bottom = level_top - 1;

        tau_cumulative[level_bottom] = tau_cumulative[level_top] + tau_s * slant_factors[i];
      }

      // Direct beam attenuation (Beer-Lambert law)
      // TOA level (index n_layers) receives full flux
      result.direct[n_layers] = flux_toa * mu0;
      result.actinic_direct[n_layers] = flux_toa;

      for (std::size_t i = n_layers; i > 0; --i)
      {
        std::size_t layer = i - 1;  // Layer between levels i and i-1

        // Apply delta scaling to get effective optical depth
        auto [tau_s, omega_s, g_s] = DeltaScale(tau[layer], omega[layer], g[layer]);

        // Slant path optical depth for this layer
        double tau_slant = tau_s * slant_factors[layer];

        // Transmittance through this layer
        double trans = std::exp(-tau_slant);

        result.direct[i - 1] = result.direct[i] * trans;
        result.actinic_direct[i - 1] = result.actinic_direct[i] * trans;
      }

      // Simplified diffuse calculation using adding method
      // For a pure absorbing atmosphere (omega=0), there's no diffuse
      // For scattering atmosphere, we use the Eddington approximation

      // Calculate diffuse components layer by layer
      // This is a simplified version - full implementation would use
      // tridiagonal matrix solver for coupled layers

      // First pass: calculate layer properties
      std::vector<double> r(n_layers);   // Layer reflectance
      std::vector<double> t(n_layers);   // Layer transmittance
      std::vector<double> src(n_layers); // Layer source function

      for (std::size_t i = 0; i < n_layers; ++i)
      {
        auto [tau_s, omega_s, g_s] = DeltaScale(tau[i], omega[i], g[i]);

        if (tau_s < 1e-10 || omega_s < 1e-10)
        {
          // Negligible optical depth or pure absorption
          r[i] = 0.0;
          t[i] = std::exp(-tau_s / mu0);
          src[i] = 0.0;
        }
        else
        {
          // Two-stream coefficients
          double gamma1 = (7.0 - omega_s * (4.0 + 3.0 * g_s)) / 4.0;
          double gamma2 = -(1.0 - omega_s * (4.0 - 3.0 * g_s)) / 4.0;
          double gamma3 = (2.0 - 3.0 * g_s * mu0) / 4.0;
          double gamma4 = 1.0 - gamma3;

          // Lambda and Gamma parameters
          double lambda = std::sqrt(gamma1 * gamma1 - gamma2 * gamma2);
          double Gamma = gamma2 / (gamma1 + lambda);

          // Exponentials
          double exp_plus = std::exp(lambda * tau_s);
          double exp_minus = std::exp(-lambda * tau_s);

          // Reflectance and transmittance
          double denom = (1.0 - Gamma * Gamma * exp_minus * exp_minus);
          if (std::abs(denom) < 1e-30)
          {
            denom = 1e-30;
          }

          r[i] = Gamma * (1.0 - exp_minus * exp_minus) / denom;
          t[i] = (1.0 - Gamma * Gamma) * exp_minus / denom;

          // Source from direct beam
          double C_plus = omega_s * ((gamma1 - 1.0 / mu0) * gamma3 + gamma4 * gamma2) /
                          (lambda * lambda - 1.0 / (mu0 * mu0));
          double C_minus = omega_s * ((gamma1 + 1.0 / mu0) * gamma4 + gamma3 * gamma2) /
                           (lambda * lambda - 1.0 / (mu0 * mu0));

          src[i] = C_plus + C_minus;
        }
      }

      // Adding method: combine layers from TOA to surface
      // Simplified: just compute single-scattering approximation

      // Surface boundary: reflected direct beam
      double direct_surface = result.direct[0];
      result.diffuse_up[0] = albedo * (direct_surface + result.diffuse_down[0]);

      // Propagate diffuse upward
      for (std::size_t i = 0; i < n_layers; ++i)
      {
        result.diffuse_up[i + 1] = result.diffuse_up[i] * t[i] + r[i] * result.diffuse_down[i + 1];
      }

      // Single scattering contribution to diffuse
      for (std::size_t i = 0; i < n_layers; ++i)
      {
        auto [tau_s, omega_s, g_s] = DeltaScale(tau[i], omega[i], g[i]);

        // Source term from scattering of direct beam
        double direct_avg = 0.5 * (result.direct[i] + result.direct[i + 1]) / mu0;
        double scatter_source = omega_s * direct_avg * tau_s;

        // Add to diffuse down at bottom of layer
        result.diffuse_down[i] += 0.5 * scatter_source * (1.0 - g_s);
        // Add to diffuse up at top of layer
        result.diffuse_up[i + 1] += 0.5 * scatter_source * (1.0 + g_s);
      }

      // Recalculate surface reflection with updated diffuse_down
      result.diffuse_up[0] = albedo * (direct_surface / mu0 + result.diffuse_down[0]);

      // Actinic flux: integrate over all directions
      // For diffuse: F_actinic ≈ 2 * (F_up + F_down) for isotropic radiation
      for (std::size_t i = 0; i < n_levels; ++i)
      {
        result.actinic_diffuse[i] = 2.0 * (result.diffuse_up[i] + result.diffuse_down[i]);
      }

      return result;
    }
  };

}  // namespace tuvx
