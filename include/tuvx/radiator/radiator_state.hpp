#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace tuvx
{
  /// @brief Optical properties state for a radiator on a grid
  ///
  /// RadiatorState holds the wavelength-dependent optical properties for
  /// an atmospheric constituent across all altitude layers. These properties
  /// are the inputs to the radiative transfer solver.
  ///
  /// The three key optical properties are:
  /// - **Optical depth** (τ): Measure of atmospheric opacity
  /// - **Single scattering albedo** (ω): Ratio of scattering to total extinction
  /// - **Asymmetry factor** (g): Forward scattering parameter (-1 to +1)
  ///
  /// For pure absorbers: ω = 0
  /// For conservative scatterers: ω = 1
  /// For isotropic scattering: g = 0
  /// For forward scattering (e.g., aerosols): g > 0
  struct RadiatorState
  {
    /// @brief Layer optical depths [n_layers][n_wavelengths]
    ///
    /// τ = σ * N * Δz where σ is cross-section, N is number density,
    /// and Δz is layer thickness.
    std::vector<std::vector<double>> optical_depth;

    /// @brief Single scattering albedo [n_layers][n_wavelengths]
    ///
    /// ω = σ_scat / σ_ext, where σ_scat is scattering cross-section
    /// and σ_ext is extinction (absorption + scattering) cross-section.
    /// Range: [0, 1]
    std::vector<std::vector<double>> single_scattering_albedo;

    /// @brief Asymmetry factor [n_layers][n_wavelengths]
    ///
    /// g = <cos(θ)> where θ is the scattering angle.
    /// Range: [-1, 1], typically [0, 1] for atmospheric particles.
    /// g = 0 for isotropic (Rayleigh) scattering.
    /// g ≈ 0.7-0.9 for typical aerosols.
    std::vector<std::vector<double>> asymmetry_factor;

    /// @brief Initialize state with zeros for given dimensions
    /// @param n_layers Number of altitude layers
    /// @param n_wavelengths Number of wavelength bins
    void Initialize(std::size_t n_layers, std::size_t n_wavelengths)
    {
      optical_depth.assign(n_layers, std::vector<double>(n_wavelengths, 0.0));
      single_scattering_albedo.assign(n_layers, std::vector<double>(n_wavelengths, 0.0));
      asymmetry_factor.assign(n_layers, std::vector<double>(n_wavelengths, 0.0));
    }

    /// @brief Get the number of layers
    std::size_t NumberOfLayers() const
    {
      return optical_depth.size();
    }

    /// @brief Get the number of wavelengths
    std::size_t NumberOfWavelengths() const
    {
      if (optical_depth.empty())
      {
        return 0;
      }
      return optical_depth[0].size();
    }

    /// @brief Check if state is empty (not initialized)
    bool Empty() const
    {
      return optical_depth.empty();
    }

    /// @brief Accumulate another radiator's state into this one
    ///
    /// Combines optical properties following radiative transfer rules:
    /// - Optical depths add: τ_total = τ_1 + τ_2
    /// - Single scattering albedos combine as weighted average by optical depth
    /// - Asymmetry factors combine as weighted average by scattering optical depth
    ///
    /// @param other The state to accumulate
    /// @throws std::runtime_error if dimensions don't match
    void Accumulate(const RadiatorState& other)
    {
      if (other.Empty())
      {
        return;
      }

      if (Empty())
      {
        *this = other;
        return;
      }

      std::size_t n_layers = NumberOfLayers();
      std::size_t n_wavelengths = NumberOfWavelengths();

      if (other.NumberOfLayers() != n_layers || other.NumberOfWavelengths() != n_wavelengths)
      {
        throw std::runtime_error("Cannot accumulate RadiatorState with different dimensions");
      }

      for (std::size_t i = 0; i < n_layers; ++i)
      {
        for (std::size_t j = 0; j < n_wavelengths; ++j)
        {
          double tau_1 = optical_depth[i][j];
          double tau_2 = other.optical_depth[i][j];
          double omega_1 = single_scattering_albedo[i][j];
          double omega_2 = other.single_scattering_albedo[i][j];
          double g_1 = asymmetry_factor[i][j];
          double g_2 = other.asymmetry_factor[i][j];

          // Combined optical depth
          double tau_total = tau_1 + tau_2;

          // Combined single scattering albedo (weighted by optical depth)
          // ω_total = (τ_1 * ω_1 + τ_2 * ω_2) / τ_total
          double omega_total = 0.0;
          if (tau_total > 0.0)
          {
            omega_total = (tau_1 * omega_1 + tau_2 * omega_2) / tau_total;
          }

          // Combined asymmetry factor (weighted by scattering optical depth)
          // g_total = (τ_1 * ω_1 * g_1 + τ_2 * ω_2 * g_2) / (τ_1 * ω_1 + τ_2 * ω_2)
          double g_total = 0.0;
          double scatter_tau_1 = tau_1 * omega_1;
          double scatter_tau_2 = tau_2 * omega_2;
          double scatter_tau_total = scatter_tau_1 + scatter_tau_2;
          if (scatter_tau_total > 0.0)
          {
            g_total = (scatter_tau_1 * g_1 + scatter_tau_2 * g_2) / scatter_tau_total;
          }

          optical_depth[i][j] = tau_total;
          single_scattering_albedo[i][j] = omega_total;
          asymmetry_factor[i][j] = g_total;
        }
      }
    }

    /// @brief Scale all optical depths by a factor
    /// @param factor Scale factor to apply
    void Scale(double factor)
    {
      for (auto& layer : optical_depth)
      {
        for (auto& tau : layer)
        {
          tau *= factor;
        }
      }
    }

    /// @brief Get total column optical depth at each wavelength
    /// @return Vector of total optical depths [n_wavelengths]
    std::vector<double> TotalOpticalDepth() const
    {
      if (Empty())
      {
        return {};
      }

      std::size_t n_wavelengths = NumberOfWavelengths();
      std::vector<double> total(n_wavelengths, 0.0);

      for (const auto& layer : optical_depth)
      {
        for (std::size_t j = 0; j < n_wavelengths; ++j)
        {
          total[j] += layer[j];
        }
      }

      return total;
    }
  };

}  // namespace tuvx
