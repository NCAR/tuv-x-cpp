#pragma once

namespace tuvx
{
  namespace constants
  {
    // Mathematical constants
    /// @brief Pi (ratio of circumference to diameter)
    static constexpr double kPi = 3.14159265358979323846;

    /// @brief 2 * Pi
    static constexpr double kTwoPi = 2.0 * kPi;

    /// @brief Pi / 2
    static constexpr double kHalfPi = kPi / 2.0;

    /// @brief Degrees to radians conversion factor
    static constexpr double kDegreesToRadians = kPi / 180.0;

    /// @brief Radians to degrees conversion factor
    static constexpr double kRadiansToDegrees = 180.0 / kPi;

    // Physical constants (SI units, 2019 CODATA values)

    /// @brief Boltzmann constant [J K^-1]
    static constexpr double kBoltzmannConstant = 1.380649e-23;

    /// @brief Avogadro constant [mol^-1]
    static constexpr double kAvogadroConstant = 6.02214076e23;

    /// @brief Universal gas constant [J K^-1 mol^-1]
    /// R = k_B * N_A
    static constexpr double kUniversalGasConstant = 8.314462618;

    /// @brief Planck constant [J s]
    static constexpr double kPlanckConstant = 6.62607015e-34;

    /// @brief Speed of light in vacuum [m s^-1]
    static constexpr double kSpeedOfLight = 2.99792458e8;

    /// @brief Elementary charge [C]
    static constexpr double kElementaryCharge = 1.602176634e-19;

    /// @brief Stefan-Boltzmann constant [W m^-2 K^-4]
    static constexpr double kStefanBoltzmannConstant = 5.670374419e-8;

    // Atmospheric and Earth constants

    /// @brief Mean radius of Earth [m]
    static constexpr double kEarthRadius = 6.371e6;

    /// @brief Solar constant (total solar irradiance) [W m^-2]
    static constexpr double kSolarConstant = 1361.0;

    /// @brief Standard atmospheric pressure [Pa]
    static constexpr double kStandardPressure = 101325.0;

    /// @brief Standard temperature [K] (0 degrees Celsius)
    static constexpr double kStandardTemperature = 273.15;

    /// @brief Loschmidt constant (number density at STP) [m^-3]
    static constexpr double kLoschmidtConstant = 2.6867774e25;

    // Unit conversions for wavelength/energy

    /// @brief Conversion factor from nm to m
    static constexpr double kNanometersToMeters = 1.0e-9;

    /// @brief Conversion factor from cm^-1 to J (using h*c)
    static constexpr double kWavenumberToJoules = kPlanckConstant * kSpeedOfLight * 100.0;

  }  // namespace constants
}  // namespace tuvx
