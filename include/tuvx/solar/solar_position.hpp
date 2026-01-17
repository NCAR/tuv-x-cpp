#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <tuvx/util/constants.hpp>

namespace tuvx
{
  namespace solar
  {
    /// @brief Convert calendar date to Julian Day Number
    /// @param year Year (1950-2050 recommended for accuracy)
    /// @param month Month (1-12)
    /// @param day Day of month (1-31)
    /// @return Julian Day Number
    ///
    /// Algorithm from The Astronomical Almanac.
    inline double JulianDay(int year, int month, int day)
    {
      // Adjust for January/February
      int a = (14 - month) / 12;
      int y = year + 4800 - a;
      int m = month + 12 * a - 3;

      // Julian Day Number
      double jd = day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;

      return jd;
    }

    /// @brief Get day of year from calendar date
    /// @param year Year
    /// @param month Month (1-12)
    /// @param day Day of month
    /// @return Day of year (1-366)
    inline int DayOfYear(int year, int month, int day)
    {
      // Days in each month (non-leap year)
      static const int days_in_month[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

      int doy = 0;
      for (int i = 1; i < month; ++i)
      {
        doy += days_in_month[i];
      }
      doy += day;

      // Leap year adjustment
      bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
      if (is_leap && month > 2)
      {
        doy += 1;
      }

      return doy;
    }

    /// @brief Calculate Earth-Sun distance correction factor
    /// @param day_of_year Day of year (1-366)
    /// @return Distance factor (r0/r)^2 where r0 = 1 AU
    ///
    /// This factor is used to adjust solar flux for the varying Earth-Sun distance.
    /// At perihelion (~Jan 3), factor > 1; at aphelion (~Jul 4), factor < 1.
    inline double EarthSunDistanceFactor(int day_of_year)
    {
      // Eccentricity correction using Spencer's formula
      // More accurate than simple cosine approximation
      double gamma = 2.0 * constants::kPi * (day_of_year - 1) / 365.0;

      double factor = 1.000110 + 0.034221 * std::cos(gamma) + 0.001280 * std::sin(gamma) +
                      0.000719 * std::cos(2.0 * gamma) + 0.000077 * std::sin(2.0 * gamma);

      return factor;
    }

    /// @brief Calculate Earth-Sun distance in Astronomical Units
    /// @param day_of_year Day of year (1-366)
    /// @return Distance in AU
    inline double EarthSunDistance(int day_of_year)
    {
      double factor = EarthSunDistanceFactor(day_of_year);
      return 1.0 / std::sqrt(factor);
    }

    /// @brief Result of solar position calculation
    struct SolarPositionResult
    {
      double zenith_angle;   ///< Solar zenith angle [degrees]
      double azimuth_angle;  ///< Solar azimuth angle [degrees, clockwise from north]
      double elevation;      ///< Solar elevation angle [degrees]
      double hour_angle;     ///< Hour angle [degrees]
      double declination;    ///< Solar declination [degrees]
    };

    /// @brief Calculate solar position using Michalsky algorithm
    /// @param year Year (1950-2050 for best accuracy)
    /// @param month Month (1-12)
    /// @param day Day of month
    /// @param hour Hour of day in UTC (0-24, can include fractional hours)
    /// @param latitude Observer latitude [degrees, positive north]
    /// @param longitude Observer longitude [degrees, positive east]
    /// @return Solar position result
    ///
    /// Based on Michalsky, J., 1988: The Astronomical Almanac's algorithm for
    /// approximate solar position (1950-2050). Solar Energy, 40, 227-235.
    ///
    /// Accuracy: 0.01 degrees for solar zenith angle.
    inline SolarPositionResult CalculateSolarPosition(
        int year,
        int month,
        int day,
        double hour,
        double latitude,
        double longitude)
    {
      // Convert to radians
      double lat_rad = latitude * constants::kDegreesToRadians;

      // Julian Day at noon
      double jd = JulianDay(year, month, day);

      // Julian Day including fractional day (hour in UTC)
      double jd_frac = jd + (hour - 12.0) / 24.0;

      // Julian centuries from J2000.0 (2000 Jan 1.5)
      double t = (jd_frac - 2451545.0) / 36525.0;

      // Mean longitude of the Sun [degrees]
      double mean_longitude = std::fmod(280.46646 + 36000.76983 * t + 0.0003032 * t * t, 360.0);
      if (mean_longitude < 0.0)
      {
        mean_longitude += 360.0;
      }

      // Mean anomaly of the Sun [degrees]
      double mean_anomaly = std::fmod(357.52911 + 35999.05029 * t - 0.0001537 * t * t, 360.0);
      if (mean_anomaly < 0.0)
      {
        mean_anomaly += 360.0;
      }
      double mean_anomaly_rad = mean_anomaly * constants::kDegreesToRadians;

      // Equation of center [degrees]
      double center = (1.914602 - 0.004817 * t - 0.000014 * t * t) * std::sin(mean_anomaly_rad) +
                      (0.019993 - 0.000101 * t) * std::sin(2.0 * mean_anomaly_rad) +
                      0.000289 * std::sin(3.0 * mean_anomaly_rad);

      // True longitude of the Sun [degrees]
      double sun_longitude = mean_longitude + center;

      // Obliquity of the ecliptic [degrees]
      double obliquity = 23.439291 - 0.0130042 * t - 0.00000016 * t * t;
      double obliquity_rad = obliquity * constants::kDegreesToRadians;

      // Sun's right ascension [radians]
      double sun_longitude_rad = sun_longitude * constants::kDegreesToRadians;
      double right_ascension =
          std::atan2(std::cos(obliquity_rad) * std::sin(sun_longitude_rad), std::cos(sun_longitude_rad));

      // Solar declination [radians]
      double declination_rad = std::asin(std::sin(obliquity_rad) * std::sin(sun_longitude_rad));

      // Greenwich Mean Sidereal Time [degrees]
      double gmst = std::fmod(280.46061837 + 360.98564736629 * (jd_frac - 2451545.0) +
                                  0.000387933 * t * t - t * t * t / 38710000.0,
                              360.0);
      if (gmst < 0.0)
      {
        gmst += 360.0;
      }

      // Local Mean Sidereal Time [degrees]
      double lmst = gmst + longitude;

      // Hour angle [degrees]
      double hour_angle = lmst - right_ascension * constants::kRadiansToDegrees;
      // Normalize to -180 to +180
      while (hour_angle > 180.0)
      {
        hour_angle -= 360.0;
      }
      while (hour_angle < -180.0)
      {
        hour_angle += 360.0;
      }
      double hour_angle_rad = hour_angle * constants::kDegreesToRadians;

      // Solar zenith angle [radians]
      double cos_zenith = std::sin(lat_rad) * std::sin(declination_rad) +
                          std::cos(lat_rad) * std::cos(declination_rad) * std::cos(hour_angle_rad);

      // Clamp to valid range
      cos_zenith = std::max(-1.0, std::min(1.0, cos_zenith));
      double zenith_rad = std::acos(cos_zenith);

      // Solar elevation
      double elevation = 90.0 - zenith_rad * constants::kRadiansToDegrees;

      // Solar azimuth [radians] - measured clockwise from north
      double sin_azimuth =
          -std::cos(declination_rad) * std::sin(hour_angle_rad) / std::sin(zenith_rad);
      double cos_azimuth =
          (std::sin(declination_rad) - std::sin(lat_rad) * cos_zenith) / (std::cos(lat_rad) * std::sin(zenith_rad));

      // Handle zenith case (sun directly overhead)
      double azimuth;
      if (std::abs(std::sin(zenith_rad)) < 1e-10)
      {
        azimuth = 0.0;
      }
      else
      {
        azimuth = std::atan2(sin_azimuth, cos_azimuth) * constants::kRadiansToDegrees;
        if (azimuth < 0.0)
        {
          azimuth += 360.0;
        }
      }

      SolarPositionResult result;
      result.zenith_angle = zenith_rad * constants::kRadiansToDegrees;
      result.azimuth_angle = azimuth;
      result.elevation = elevation;
      result.hour_angle = hour_angle;
      result.declination = declination_rad * constants::kRadiansToDegrees;

      return result;
    }

    /// @brief Calculate solar zenith angle (convenience function)
    /// @param year Year
    /// @param month Month (1-12)
    /// @param day Day of month
    /// @param hour Hour in UTC (0-24)
    /// @param latitude Latitude [degrees]
    /// @param longitude Longitude [degrees]
    /// @return Solar zenith angle [degrees]
    inline double SolarZenithAngle(int year, int month, int day, double hour, double latitude, double longitude)
    {
      return CalculateSolarPosition(year, month, day, hour, latitude, longitude).zenith_angle;
    }

    /// @brief Calculate cosine of solar zenith angle
    /// @param zenith_angle Solar zenith angle [degrees]
    /// @return Cosine of zenith angle (mu_0), clamped to [-1, 1]
    inline double CosineZenithAngle(double zenith_angle)
    {
      return std::cos(zenith_angle * constants::kDegreesToRadians);
    }

    /// @brief Check if sun is above horizon
    /// @param zenith_angle Solar zenith angle [degrees]
    /// @return True if sun is above horizon (SZA < 90)
    inline bool SunAboveHorizon(double zenith_angle)
    {
      return zenith_angle < 90.0;
    }

    /// @brief Check if twilight conditions (sun below horizon but light present)
    /// @param zenith_angle Solar zenith angle [degrees]
    /// @param twilight_limit Lower limit for twilight calculations [degrees, default 108]
    /// @return True if in twilight zone (90 <= SZA <= twilight_limit)
    inline bool IsTwilight(double zenith_angle, double twilight_limit = 108.0)
    {
      return zenith_angle >= 90.0 && zenith_angle <= twilight_limit;
    }

  }  // namespace solar
}  // namespace tuvx
