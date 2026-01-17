#pragma once

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include <tuvx/grid/grid.hpp>
#include <tuvx/interpolation/linear_interpolator.hpp>
#include <tuvx/solar/solar_position.hpp>
#include <tuvx/util/constants.hpp>

namespace tuvx
{
  namespace solar
  {
    /// @brief Extraterrestrial solar flux at top of atmosphere
    ///
    /// Provides the solar spectral irradiance at 1 AU, which can be adjusted
    /// for Earth-Sun distance variations. The flux is given in photons/cm^2/s
    /// for photochemistry applications.
    class ExtraterrestrialFlux
    {
     public:
      /// @brief Construct from reference wavelengths and flux values
      /// @param wavelengths Reference wavelengths [nm]
      /// @param flux Spectral flux values [photons/cm^2/s/nm] at 1 AU
      ExtraterrestrialFlux(std::vector<double> wavelengths, std::vector<double> flux)
          : wavelengths_(std::move(wavelengths)), flux_(std::move(flux))
      {
        if (wavelengths_.size() != flux_.size())
        {
          throw std::invalid_argument("Wavelength and flux arrays must have same size");
        }
        if (wavelengths_.size() < 2)
        {
          throw std::invalid_argument("At least 2 wavelength points required");
        }
      }

      /// @brief Calculate flux on a wavelength grid
      /// @param wavelength_grid Target wavelength grid
      /// @param earth_sun_distance_factor Factor (r0/r)^2 to adjust for distance
      /// @return Spectral flux at grid midpoints [photons/cm^2/s/nm]
      std::vector<double> Calculate(const Grid& wavelength_grid, double earth_sun_distance_factor = 1.0) const
      {
        std::size_t n_wavelengths = wavelength_grid.Spec().n_cells;
        auto target_wavelengths = wavelength_grid.Midpoints();

        LinearInterpolator interp;
        std::vector<double> result = interp.Interpolate(target_wavelengths, wavelengths_, flux_);

        // Zero out values outside reference range
        double wl_min = wavelengths_.front();
        double wl_max = wavelengths_.back();

        for (std::size_t i = 0; i < n_wavelengths; ++i)
        {
          if (target_wavelengths[i] < wl_min || target_wavelengths[i] > wl_max)
          {
            result[i] = 0.0;
          }
          else
          {
            // Apply distance correction
            result[i] *= earth_sun_distance_factor;
          }
        }

        return result;
      }

      /// @brief Calculate integrated flux over wavelength bins
      /// @param wavelength_grid Target wavelength grid
      /// @param earth_sun_distance_factor Factor (r0/r)^2 to adjust for distance
      /// @return Integrated flux per bin [photons/cm^2/s]
      std::vector<double> CalculateIntegrated(const Grid& wavelength_grid, double earth_sun_distance_factor = 1.0)
          const
      {
        auto spectral_flux = Calculate(wavelength_grid, earth_sun_distance_factor);
        auto deltas = wavelength_grid.Deltas();

        std::vector<double> integrated(spectral_flux.size());
        for (std::size_t i = 0; i < spectral_flux.size(); ++i)
        {
          integrated[i] = spectral_flux[i] * std::abs(deltas[i]);
        }

        return integrated;
      }

      /// @brief Get reference wavelengths
      const std::vector<double>& ReferenceWavelengths() const
      {
        return wavelengths_;
      }

      /// @brief Get reference flux values
      const std::vector<double>& ReferenceFlux() const
      {
        return flux_;
      }

     private:
      std::vector<double> wavelengths_;
      std::vector<double> flux_;
    };

    /// @brief Convert irradiance from W/m^2/nm to photons/cm^2/s/nm
    /// @param irradiance Spectral irradiance [W/m^2/nm]
    /// @param wavelength Wavelength [nm]
    /// @return Photon flux [photons/cm^2/s/nm]
    ///
    /// Uses: E = hc/λ, so n_photons = E * λ / (hc)
    /// With unit conversions: W/m^2/nm -> photons/cm^2/s/nm
    inline double IrradianceToPhotonFlux(double irradiance, double wavelength)
    {
      // E_photon = hc/λ where λ is in meters
      // n_photons = Power / E_photon = Power * λ / (hc)

      // Convert wavelength from nm to m
      double wavelength_m = wavelength * 1.0e-9;

      // Photons per Joule at this wavelength
      double photons_per_joule = wavelength_m / (constants::kPlanckConstant * constants::kSpeedOfLight);

      // Convert W/m^2 to W/cm^2 (factor of 1e-4)
      // irradiance [W/m^2/nm] * 1e-4 [m^2/cm^2] = [W/cm^2/nm]
      // Then multiply by photons_per_joule to get [photons/cm^2/s/nm]
      return irradiance * 1.0e-4 * photons_per_joule;
    }

    /// @brief Convert photon flux to irradiance
    /// @param photon_flux Photon flux [photons/cm^2/s/nm]
    /// @param wavelength Wavelength [nm]
    /// @return Spectral irradiance [W/m^2/nm]
    inline double PhotonFluxToIrradiance(double photon_flux, double wavelength)
    {
      double wavelength_m = wavelength * 1.0e-9;
      double energy_per_photon = constants::kPlanckConstant * constants::kSpeedOfLight / wavelength_m;

      // Convert photons/cm^2/s/nm to W/cm^2/nm then to W/m^2/nm
      return photon_flux * energy_per_photon * 1.0e4;
    }

    /// @brief Create a simple blackbody solar spectrum (approximate)
    /// @param wavelengths Wavelengths to calculate [nm]
    /// @param temperature Effective temperature [K], default 5778 K
    /// @return Spectral flux [photons/cm^2/s/nm] at 1 AU
    ///
    /// This is a simplified Planck function scaled to match the solar constant.
    /// For accurate calculations, use measured solar spectra.
    inline std::vector<double> BlackbodySolarFlux(const std::vector<double>& wavelengths, double temperature = 5778.0)
    {
      std::vector<double> flux(wavelengths.size());

      // Solar radius and distance
      constexpr double solar_radius = 6.96e8;          // m
      constexpr double earth_sun_distance = 1.496e11;  // m (1 AU)
      double solid_angle_factor = std::pow(solar_radius / earth_sun_distance, 2);

      for (std::size_t i = 0; i < wavelengths.size(); ++i)
      {
        double wavelength_m = wavelengths[i] * 1.0e-9;

        // Planck function B(λ,T) [W/m^2/sr/m]
        double x = constants::kPlanckConstant * constants::kSpeedOfLight / (wavelength_m * constants::kBoltzmannConstant * temperature);

        double planck;
        if (x > 700.0)
        {
          planck = 0.0;  // Avoid overflow
        }
        else
        {
          planck = (2.0 * constants::kPlanckConstant * std::pow(constants::kSpeedOfLight, 2)) /
                   (std::pow(wavelength_m, 5) * (std::exp(x) - 1.0));
        }

        // Irradiance at Earth [W/m^2/m] = B * π * (R_sun/d)^2
        double irradiance = planck * constants::kPi * solid_angle_factor;

        // Convert to W/m^2/nm (divide by 1e9 m/nm)
        irradiance *= 1.0e-9;

        // Convert to photons/cm^2/s/nm
        flux[i] = IrradianceToPhotonFlux(irradiance, wavelengths[i]);
      }

      return flux;
    }

    /// @brief Standard solar spectrum reference data
    ///
    /// These are representative values for testing and simple applications.
    /// For research applications, use measured spectra from sources like
    /// SORCE, TSIS, or the WMO reference spectrum.
    namespace reference_spectra
    {
      /// @brief Get ASTM E-490 AM0 reference spectrum (simplified)
      /// @return Pair of (wavelengths [nm], flux [photons/cm^2/s/nm])
      ///
      /// This is a simplified representation with key wavelengths.
      /// Values are approximate and suitable for testing.
      inline std::pair<std::vector<double>, std::vector<double>> ASTM_E490_AM0_Simplified()
      {
        // Representative wavelengths from UV through visible
        std::vector<double> wavelengths = { 200.0,  220.0,  240.0,  260.0,  280.0,  300.0,  320.0,  340.0,
                                            360.0,  380.0,  400.0,  450.0,  500.0,  550.0,  600.0,  650.0,
                                            700.0,  750.0,  800.0,  850.0,  900.0,  950.0,  1000.0 };

        // Approximate spectral irradiance [W/m^2/nm] from ASTM E-490
        std::vector<double> irradiance = { 0.01,  0.02,  0.03,  0.06,  0.15,  0.48,  0.71,  0.95,
                                           1.08,  1.15,  1.52,  2.05,  1.95,  1.85,  1.77,  1.65,
                                           1.49,  1.30,  1.13,  0.98,  0.85,  0.80,  0.75 };

        // Convert to photon flux
        std::vector<double> flux(wavelengths.size());
        for (std::size_t i = 0; i < wavelengths.size(); ++i)
        {
          flux[i] = IrradianceToPhotonFlux(irradiance[i], wavelengths[i]);
        }

        return { wavelengths, flux };
      }

      /// @brief Create ExtraterrestrialFlux from ASTM E-490 reference
      inline ExtraterrestrialFlux CreateASTM_E490()
      {
        auto [wavelengths, flux] = ASTM_E490_AM0_Simplified();
        return ExtraterrestrialFlux(std::move(wavelengths), std::move(flux));
      }
    }  // namespace reference_spectra

  }  // namespace solar
}  // namespace tuvx
