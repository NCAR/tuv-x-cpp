#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <tuvx/grid/grid.hpp>
#include <tuvx/profile/profile.hpp>
#include <tuvx/radiator/radiator.hpp>

namespace tuvx
{
  /// @brief Aerosol radiator with configurable optical properties
  ///
  /// Computes aerosol optical properties using an Ångström-type parameterization
  /// for the wavelength dependence of optical depth:
  ///
  ///   τ(λ) = τ_ref × (λ/λ_ref)^(-α)
  ///
  /// where τ_ref is the reference optical depth at λ_ref (typically 550 nm),
  /// and α is the Ångström exponent (typically 0.5-2.5).
  ///
  /// The vertical distribution follows an exponential decay:
  ///   τ(z) = τ_total × exp(-z/H) / H
  ///
  /// where H is the aerosol scale height (typically 1-2 km for boundary layer,
  /// 5-8 km for background aerosol).
  ///
  /// Single scattering albedo and asymmetry factor are configurable and can
  /// be wavelength-dependent.
  class AerosolRadiator : public Radiator
  {
   public:
    /// @brief Aerosol configuration parameters
    struct Config
    {
      /// Reference optical depth at reference wavelength
      double optical_depth_ref{ 0.1 };

      /// Reference wavelength [nm]
      double wavelength_ref{ 550.0 };

      /// Ångström exponent for wavelength dependence
      double angstrom_exponent{ 1.3 };

      /// Aerosol scale height [km]
      double scale_height{ 2.0 };

      /// Single scattering albedo (0-1)
      /// If wavelength_dependent_ssa is empty, this uniform value is used
      double single_scattering_albedo{ 0.9 };

      /// Asymmetry factor (-1 to 1)
      /// If wavelength_dependent_g is empty, this uniform value is used
      double asymmetry_factor{ 0.7 };

      /// Optional wavelength-dependent single scattering albedo
      /// First element: wavelengths [nm], second element: ssa values
      std::vector<double> ssa_wavelengths{};
      std::vector<double> ssa_values{};

      /// Optional wavelength-dependent asymmetry factor
      /// First element: wavelengths [nm], second element: g values
      std::vector<double> g_wavelengths{};
      std::vector<double> g_values{};
    };

    /// @brief Construct aerosol radiator with default configuration
    AerosolRadiator()
    {
      name_ = "aerosol";
    }

    /// @brief Construct aerosol radiator with custom configuration
    /// @param config Aerosol configuration parameters
    /// @param wavelength_grid_name Name of the wavelength grid in warehouse
    /// @param altitude_grid_name Name of the altitude grid in warehouse
    explicit AerosolRadiator(
        Config config,
        std::string wavelength_grid_name = "wavelength",
        std::string altitude_grid_name = "altitude")
        : config_(std::move(config)),
          wavelength_grid_name_(std::move(wavelength_grid_name)),
          altitude_grid_name_(std::move(altitude_grid_name))
    {
      name_ = "aerosol";
    }

    /// @brief Clone this radiator
    std::unique_ptr<Radiator> Clone() const override
    {
      return std::make_unique<AerosolRadiator>(config_, wavelength_grid_name_, altitude_grid_name_);
    }

    /// @brief Get current configuration
    const Config& GetConfig() const
    {
      return config_;
    }

    /// @brief Set configuration
    void SetConfig(Config config)
    {
      config_ = std::move(config);
    }

    /// @brief Update optical state from current atmospheric conditions
    ///
    /// Computes aerosol optical properties at each layer and wavelength.
    void UpdateState(const GridWarehouse& grids, const ProfileWarehouse& profiles) override
    {
      (void)profiles;  // Aerosol doesn't use atmospheric profiles

      // Get grids
      const auto& wl_grid = grids.Get(wavelength_grid_name_, "nm");
      const auto& alt_grid = grids.Get(altitude_grid_name_, "km");

      std::size_t n_layers = alt_grid.Spec().n_cells;
      std::size_t n_wavelengths = wl_grid.Spec().n_cells;

      // Initialize state
      state_.Initialize(n_layers, n_wavelengths);

      // Get wavelength midpoints and altitude edges
      auto wavelengths = wl_grid.Midpoints();
      auto alt_edges = alt_grid.Edges();

      // Compute optical properties for each layer and wavelength
      for (std::size_t i = 0; i < n_layers; ++i)
      {
        // Layer altitude bounds
        double z_low = alt_edges[i];
        double z_high = alt_edges[i + 1];
        if (z_low > z_high)
        {
          std::swap(z_low, z_high);
        }

        // Vertical distribution factor: fraction of total OD in this layer
        // Using exponential decay: τ(z) ∝ exp(-z/H)
        // Integral from z_low to z_high: H × [exp(-z_low/H) - exp(-z_high/H)]
        double H = config_.scale_height;
        double vert_factor = std::exp(-z_low / H) - std::exp(-z_high / H);

        for (std::size_t j = 0; j < n_wavelengths; ++j)
        {
          // Wavelength dependence: τ(λ) = τ_ref × (λ/λ_ref)^(-α)
          double wl_ratio = wavelengths[j] / config_.wavelength_ref;
          double tau_spectral = config_.optical_depth_ref * std::pow(wl_ratio, -config_.angstrom_exponent);

          // Layer optical depth
          state_.optical_depth[i][j] = tau_spectral * vert_factor;

          // Single scattering albedo
          state_.single_scattering_albedo[i][j] = GetSSA(wavelengths[j]);

          // Asymmetry factor
          state_.asymmetry_factor[i][j] = GetAsymmetry(wavelengths[j]);
        }
      }
    }

   private:
    Config config_;
    std::string wavelength_grid_name_{ "wavelength" };
    std::string altitude_grid_name_{ "altitude" };

    /// @brief Get single scattering albedo at given wavelength
    double GetSSA(double wavelength_nm) const
    {
      if (config_.ssa_wavelengths.empty() || config_.ssa_values.empty())
      {
        return config_.single_scattering_albedo;
      }
      return InterpolateValue(wavelength_nm, config_.ssa_wavelengths, config_.ssa_values);
    }

    /// @brief Get asymmetry factor at given wavelength
    double GetAsymmetry(double wavelength_nm) const
    {
      if (config_.g_wavelengths.empty() || config_.g_values.empty())
      {
        return config_.asymmetry_factor;
      }
      return InterpolateValue(wavelength_nm, config_.g_wavelengths, config_.g_values);
    }

    /// @brief Linear interpolation helper
    static double InterpolateValue(
        double x,
        const std::vector<double>& x_data,
        const std::vector<double>& y_data)
    {
      if (x_data.size() < 2 || y_data.size() < 2)
      {
        return y_data.empty() ? 0.0 : y_data[0];
      }

      // Extrapolate with constant value outside range
      if (x <= x_data.front())
      {
        return y_data.front();
      }
      if (x >= x_data.back())
      {
        return y_data.back();
      }

      // Find bracketing interval
      for (std::size_t i = 0; i < x_data.size() - 1; ++i)
      {
        if (x >= x_data[i] && x < x_data[i + 1])
        {
          double t = (x - x_data[i]) / (x_data[i + 1] - x_data[i]);
          return y_data[i] + t * (y_data[i + 1] - y_data[i]);
        }
      }

      return y_data.back();
    }
  };

}  // namespace tuvx
