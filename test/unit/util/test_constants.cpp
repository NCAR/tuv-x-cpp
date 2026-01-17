#include <tuvx/util/constants.hpp>

#include <cmath>

#include <gtest/gtest.h>

namespace
{
  // Tolerance for floating-point comparisons
  constexpr double RELATIVE_TOLERANCE = 1e-10;

  bool IsClose(double a, double b, double rel_tol = RELATIVE_TOLERANCE)
  {
    return std::abs(a - b) <= rel_tol * std::max(std::abs(a), std::abs(b));
  }
}  // namespace

TEST(ConstantsTest, MathematicalConstants)
{
  // Test Pi value
  EXPECT_NEAR(tuvx::constants::kPi, 3.14159265358979323846, 1e-15);

  // Test derived constants
  EXPECT_NEAR(tuvx::constants::kTwoPi, 2.0 * tuvx::constants::kPi, 1e-15);
  EXPECT_NEAR(tuvx::constants::kHalfPi, tuvx::constants::kPi / 2.0, 1e-15);

  // Test degree/radian conversions
  EXPECT_NEAR(tuvx::constants::kDegreesToRadians * 180.0, tuvx::constants::kPi, 1e-15);
  EXPECT_NEAR(tuvx::constants::kRadiansToDegrees * tuvx::constants::kPi, 180.0, 1e-15);

  // Test that conversions are inverses of each other
  EXPECT_NEAR(tuvx::constants::kDegreesToRadians * tuvx::constants::kRadiansToDegrees, 1.0, 1e-15);
}

TEST(ConstantsTest, PhysicalConstants)
{
  // Test Boltzmann constant (2019 CODATA exact value)
  EXPECT_EQ(tuvx::constants::kBoltzmannConstant, 1.380649e-23);

  // Test Avogadro constant (2019 CODATA exact value)
  EXPECT_EQ(tuvx::constants::kAvogadroConstant, 6.02214076e23);

  // Test Planck constant (2019 CODATA exact value)
  EXPECT_EQ(tuvx::constants::kPlanckConstant, 6.62607015e-34);

  // Test speed of light (exact by definition)
  EXPECT_EQ(tuvx::constants::kSpeedOfLight, 2.99792458e8);

  // Test elementary charge (2019 CODATA exact value)
  EXPECT_EQ(tuvx::constants::kElementaryCharge, 1.602176634e-19);
}

TEST(ConstantsTest, UniversalGasConstantRelationship)
{
  // Verify R = k_B * N_A
  double calculated_R = tuvx::constants::kBoltzmannConstant * tuvx::constants::kAvogadroConstant;
  EXPECT_TRUE(IsClose(tuvx::constants::kUniversalGasConstant, calculated_R))
      << "R should equal k_B * N_A: " << tuvx::constants::kUniversalGasConstant << " vs " << calculated_R;
}

TEST(ConstantsTest, AtmosphericConstants)
{
  // Test Earth radius (approximate value)
  EXPECT_EQ(tuvx::constants::kEarthRadius, 6.371e6);

  // Test solar constant (approximate value, varies slightly)
  EXPECT_EQ(tuvx::constants::kSolarConstant, 1361.0);

  // Test standard pressure (exact by definition)
  EXPECT_EQ(tuvx::constants::kStandardPressure, 101325.0);

  // Test standard temperature (exact by definition: 0 C)
  EXPECT_EQ(tuvx::constants::kStandardTemperature, 273.15);
}

TEST(ConstantsTest, LoschmidtConstantRelationship)
{
  // Loschmidt constant = N_A / V_m = P / (k_B * T) at STP
  // n = P / (k_B * T)
  double calculated_loschmidt = tuvx::constants::kStandardPressure /
                                (tuvx::constants::kBoltzmannConstant * tuvx::constants::kStandardTemperature);

  // Allow larger tolerance since Loschmidt is derived
  EXPECT_TRUE(IsClose(tuvx::constants::kLoschmidtConstant, calculated_loschmidt, 1e-5))
      << "Loschmidt constant relationship: " << tuvx::constants::kLoschmidtConstant << " vs " << calculated_loschmidt;
}

TEST(ConstantsTest, UnitConversions)
{
  // Test nm to m conversion
  EXPECT_EQ(tuvx::constants::kNanometersToMeters, 1.0e-9);

  // Test that 1 nm = 1e-9 m
  double wavelength_nm = 500.0;  // 500 nm (visible light)
  double wavelength_m = wavelength_nm * tuvx::constants::kNanometersToMeters;
  EXPECT_DOUBLE_EQ(wavelength_m, 5.0e-7);

  // Test wavenumber to energy conversion uses h*c
  double expected_conversion = tuvx::constants::kPlanckConstant * tuvx::constants::kSpeedOfLight * 100.0;
  EXPECT_EQ(tuvx::constants::kWavenumberToJoules, expected_conversion);
}

TEST(ConstantsTest, StefanBoltzmannConstant)
{
  // Stefan-Boltzmann constant value
  EXPECT_NEAR(tuvx::constants::kStefanBoltzmannConstant, 5.670374419e-8, 1e-17);
}
