#pragma once

#include <string>
#include <vector>

#include <tuvx/util/constants.hpp>

namespace tuvx
{
  /// @brief Configuration for TUV model calculation
  ///
  /// Specifies all parameters needed to set up and run a TUV calculation.
  struct ModelConfig
  {
    // ========================================================================
    // Wavelength Grid
    // ========================================================================

    /// Wavelength grid edges [nm]
    /// If empty, a default grid will be used
    std::vector<double> wavelength_edges{};

    /// Default wavelength range [nm]
    double wavelength_min{ 280.0 };
    double wavelength_max{ 700.0 };
    std::size_t n_wavelength_bins{ 140 };

    // ========================================================================
    // Altitude Grid
    // ========================================================================

    /// Altitude grid edges [km]
    /// If empty, a default grid will be used
    std::vector<double> altitude_edges{};

    /// Default altitude range [km]
    double altitude_min{ 0.0 };
    double altitude_max{ 80.0 };
    std::size_t n_altitude_layers{ 80 };

    // ========================================================================
    // Solar Parameters
    // ========================================================================

    /// Solar zenith angle [degrees]
    /// Used if calculating for a specific SZA
    double solar_zenith_angle{ 0.0 };

    /// Day of year [1-366]
    /// Used for Earth-Sun distance correction
    int day_of_year{ 1 };

    /// Earth-Sun distance [AU]
    /// If <= 0, calculated from day_of_year
    double earth_sun_distance{ -1.0 };

    // ========================================================================
    // Geographic Location (for solar position calculation)
    // ========================================================================

    /// Latitude [degrees, -90 to 90]
    double latitude{ 0.0 };

    /// Longitude [degrees, -180 to 180]
    double longitude{ 0.0 };

    // ========================================================================
    // Surface Properties
    // ========================================================================

    /// Surface altitude [km]
    double surface_altitude{ 0.0 };

    /// Surface albedo [0-1]
    /// Can be wavelength-dependent if surface_albedo_spectrum is provided
    double surface_albedo{ 0.1 };

    /// Wavelength-dependent surface albedo
    /// If empty, uniform surface_albedo is used
    std::vector<double> surface_albedo_spectrum{};

    // ========================================================================
    // Atmospheric Profiles
    // ========================================================================

    /// Temperature profile at layer midpoints [K]
    std::vector<double> temperature_profile{};

    /// Pressure profile at layer midpoints [hPa]
    std::vector<double> pressure_profile{};

    /// Air density profile at layer midpoints [molecules/cm³]
    std::vector<double> air_density_profile{};

    /// Ozone column density profile [molecules/cm²] per layer
    std::vector<double> ozone_profile{};

    // ========================================================================
    // Solver Options
    // ========================================================================

    /// Solver type ("delta_eddington", "discrete_ordinates", etc.)
    std::string solver_type{ "delta_eddington" };

    /// Use spherical geometry corrections
    bool use_spherical_geometry{ true };

    /// Include refraction effects (future)
    bool use_refraction{ false };

    // ========================================================================
    // Earth Parameters
    // ========================================================================

    /// Earth radius [km]
    double earth_radius{ constants::kEarthRadius };

    // ========================================================================
    // Output Options
    // ========================================================================

    /// Calculate actinic flux (needed for photolysis)
    bool calculate_actinic_flux{ true };

    /// Calculate irradiance
    bool calculate_irradiance{ true };

    // ========================================================================
    // Helper Methods
    // ========================================================================

    /// Check if configuration is valid
    bool IsValid() const
    {
      // Must have positive wavelength range
      if (wavelength_min >= wavelength_max)
        return false;
      if (n_wavelength_bins == 0)
        return false;

      // Must have positive altitude range
      if (altitude_min >= altitude_max)
        return false;
      if (n_altitude_layers == 0)
        return false;

      // Surface albedo must be in [0, 1]
      if (surface_albedo < 0.0 || surface_albedo > 1.0)
        return false;

      // SZA must be in valid range
      if (solar_zenith_angle < 0.0 || solar_zenith_angle > 180.0)
        return false;

      return true;
    }

    /// Get effective Earth-Sun distance
    double EffectiveEarthSunDistance() const
    {
      if (earth_sun_distance > 0.0)
      {
        return earth_sun_distance;
      }
      // Calculate from day of year using simple formula
      // More accurate: use SolarPosition::EarthSunDistance()
      double day_angle = 2.0 * constants::kPi * (day_of_year - 1) / 365.0;
      return 1.0 - 0.01673 * std::cos(day_angle);
    }

    /// Check if calculation is for daytime
    bool IsDaytime() const
    {
      return solar_zenith_angle < 90.0;
    }

    /// Check if calculation is for twilight
    bool IsTwilight() const
    {
      return solar_zenith_angle >= 90.0 && solar_zenith_angle < 108.0;
    }
  };

  /// @brief Standard atmosphere presets
  namespace StandardAtmosphere
  {
    /// US Standard Atmosphere 1976 temperature profile [K]
    /// Returns temperature at given altitude [km]
    inline double Temperature(double altitude_km)
    {
      if (altitude_km < 11.0)
      {
        // Troposphere: lapse rate -6.5 K/km
        return 288.15 - 6.5 * altitude_km;
      }
      else if (altitude_km < 20.0)
      {
        // Lower stratosphere: isothermal
        return 216.65;
      }
      else if (altitude_km < 32.0)
      {
        // Middle stratosphere: lapse rate +1.0 K/km
        return 216.65 + 1.0 * (altitude_km - 20.0);
      }
      else if (altitude_km < 47.0)
      {
        // Upper stratosphere: lapse rate +2.8 K/km
        return 228.65 + 2.8 * (altitude_km - 32.0);
      }
      else if (altitude_km < 51.0)
      {
        // Stratopause: isothermal
        return 270.65;
      }
      else if (altitude_km < 71.0)
      {
        // Mesosphere: lapse rate -2.8 K/km
        return 270.65 - 2.8 * (altitude_km - 51.0);
      }
      else
      {
        // Upper mesosphere: lapse rate -2.0 K/km
        return 214.65 - 2.0 * (altitude_km - 71.0);
      }
    }

    /// US Standard Atmosphere 1976 pressure profile [hPa]
    /// Returns pressure at given altitude [km]
    inline double Pressure(double altitude_km)
    {
      // Barometric formula with standard atmosphere parameters
      // P = P0 * exp(-Mg*h / RT) simplified for segments

      if (altitude_km < 11.0)
      {
        // Troposphere
        double T = Temperature(altitude_km);
        return 1013.25 * std::pow(T / 288.15, 5.2559);
      }
      else if (altitude_km < 20.0)
      {
        // Lower stratosphere (isothermal)
        double P11 = 226.32;  // Pressure at 11 km
        return P11 * std::exp(-0.1577 * (altitude_km - 11.0));
      }
      else if (altitude_km < 32.0)
      {
        double P20 = 54.75;
        double T = Temperature(altitude_km);
        return P20 * std::pow(T / 216.65, -34.163);
      }
      else if (altitude_km < 47.0)
      {
        double P32 = 8.68;
        double T = Temperature(altitude_km);
        return P32 * std::pow(T / 228.65, -12.201);
      }
      else
      {
        // Approximate for upper atmosphere
        double P47 = 1.11;
        return P47 * std::exp(-0.15 * (altitude_km - 47.0));
      }
    }

    /// Air density from temperature and pressure [molecules/cm³]
    inline double AirDensity(double temperature_K, double pressure_hPa)
    {
      // n = P / (k * T), convert to molecules/cm³
      // P in Pa = pressure_hPa * 100
      // k = 1.380649e-23 J/K
      // Result in molecules/m³, then convert to /cm³
      double P_Pa = pressure_hPa * 100.0;
      double n_per_m3 = P_Pa / (constants::kBoltzmannConstant * temperature_K);
      return n_per_m3 * 1e-6;  // molecules/cm³
    }

    /// Generate standard temperature profile for altitude grid
    inline std::vector<double> GenerateTemperatureProfile(const std::vector<double>& altitude_midpoints)
    {
      std::vector<double> temps;
      temps.reserve(altitude_midpoints.size());
      for (double z : altitude_midpoints)
      {
        temps.push_back(Temperature(z));
      }
      return temps;
    }

    /// Generate standard pressure profile for altitude grid
    inline std::vector<double> GeneratePressureProfile(const std::vector<double>& altitude_midpoints)
    {
      std::vector<double> pressures;
      pressures.reserve(altitude_midpoints.size());
      for (double z : altitude_midpoints)
      {
        pressures.push_back(Pressure(z));
      }
      return pressures;
    }

    /// Generate standard air density profile for altitude grid
    inline std::vector<double> GenerateAirDensityProfile(const std::vector<double>& altitude_midpoints)
    {
      std::vector<double> densities;
      densities.reserve(altitude_midpoints.size());
      for (double z : altitude_midpoints)
      {
        double T = Temperature(z);
        double P = Pressure(z);
        densities.push_back(AirDensity(T, P));
      }
      return densities;
    }

  }  // namespace StandardAtmosphere

}  // namespace tuvx
