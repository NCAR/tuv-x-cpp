#include <tuvx/solar/solar_position.hpp>

#include <cmath>

#include <gtest/gtest.h>

using namespace tuvx::solar;

// ============================================================================
// Julian Day Tests
// ============================================================================

TEST(JulianDayTest, J2000Epoch)
{
  // J2000.0 = January 1, 2000 at noon = JD 2451545.0
  double jd = JulianDay(2000, 1, 1);
  EXPECT_NEAR(jd, 2451545.0, 0.5);  // JulianDay returns at noon
}

TEST(JulianDayTest, KnownDates)
{
  // Some known Julian Day values
  EXPECT_NEAR(JulianDay(1970, 1, 1), 2440588.0, 0.5);   // Unix epoch
  EXPECT_NEAR(JulianDay(2024, 6, 21), 2460483.0, 0.5);  // Summer solstice 2024
}

// ============================================================================
// Day of Year Tests
// ============================================================================

TEST(DayOfYearTest, January1)
{
  EXPECT_EQ(DayOfYear(2024, 1, 1), 1);
}

TEST(DayOfYearTest, December31NonLeap)
{
  EXPECT_EQ(DayOfYear(2023, 12, 31), 365);
}

TEST(DayOfYearTest, December31Leap)
{
  EXPECT_EQ(DayOfYear(2024, 12, 31), 366);
}

TEST(DayOfYearTest, March1NonLeap)
{
  // Jan (31) + Feb (28) + 1 = 60
  EXPECT_EQ(DayOfYear(2023, 3, 1), 60);
}

TEST(DayOfYearTest, March1Leap)
{
  // Jan (31) + Feb (29) + 1 = 61
  EXPECT_EQ(DayOfYear(2024, 3, 1), 61);
}

// ============================================================================
// Earth-Sun Distance Tests
// ============================================================================

TEST(EarthSunDistanceTest, Perihelion)
{
  // Perihelion is around Jan 3, distance ~0.983 AU
  double distance = EarthSunDistance(3);
  EXPECT_LT(distance, 1.0);
  EXPECT_GT(distance, 0.98);
}

TEST(EarthSunDistanceTest, Aphelion)
{
  // Aphelion is around Jul 4 (day ~185), distance ~1.017 AU
  double distance = EarthSunDistance(185);
  EXPECT_GT(distance, 1.0);
  EXPECT_LT(distance, 1.02);
}

TEST(EarthSunDistanceTest, MeanDistance)
{
  // Average over year should be close to 1 AU
  double sum = 0.0;
  for (int day = 1; day <= 365; ++day)
  {
    sum += EarthSunDistance(day);
  }
  double mean = sum / 365.0;
  EXPECT_NEAR(mean, 1.0, 0.001);
}

TEST(EarthSunDistanceFactorTest, Perihelion)
{
  // At perihelion, factor > 1 (more flux)
  double factor = EarthSunDistanceFactor(3);
  EXPECT_GT(factor, 1.0);
}

TEST(EarthSunDistanceFactorTest, Aphelion)
{
  // At aphelion, factor < 1 (less flux)
  double factor = EarthSunDistanceFactor(185);
  EXPECT_LT(factor, 1.0);
}

// ============================================================================
// Solar Position Tests
// ============================================================================

TEST(SolarPositionTest, SummerSolsticeNoon)
{
  // Summer solstice at solar noon at equator
  // Date: June 21, 2024
  // At solar noon (12:00 UTC) at longitude 0, SZA ≈ declination ≈ 23.4°
  auto result = CalculateSolarPosition(2024, 6, 21, 12.0, 0.0, 0.0);

  // At equator on summer solstice, zenith angle ≈ 23.4° (declination)
  EXPECT_NEAR(result.zenith_angle, 23.4, 1.0);
  EXPECT_NEAR(result.declination, 23.4, 0.5);
}

TEST(SolarPositionTest, WinterSolsticeNoon)
{
  // Winter solstice at solar noon at equator
  // Date: December 21, 2024
  auto result = CalculateSolarPosition(2024, 12, 21, 12.0, 0.0, 0.0);

  // At equator on winter solstice, declination ≈ -23.4°
  EXPECT_NEAR(result.declination, -23.4, 0.5);
}

TEST(SolarPositionTest, EquinoxNoon)
{
  // Spring equinox at solar noon at equator
  // Date: March 20, 2024
  auto result = CalculateSolarPosition(2024, 3, 20, 12.0, 0.0, 0.0);

  // At equator on equinox, zenith angle ≈ 0° (sun directly overhead)
  EXPECT_NEAR(result.zenith_angle, 0.0, 2.0);
  EXPECT_NEAR(result.declination, 0.0, 1.0);
}

TEST(SolarPositionTest, HighLatitudeSummer)
{
  // Boulder, CO (40°N, 105°W) on June 21 at local solar noon
  // Local solar noon at 105°W is approximately 12:00 + 7h = 19:00 UTC
  auto result = CalculateSolarPosition(2024, 6, 21, 19.0, 40.0, -105.0);

  // At 40°N on summer solstice, noon zenith angle ≈ 40 - 23.4 ≈ 16.6°
  EXPECT_NEAR(result.zenith_angle, 16.6, 3.0);
  EXPECT_TRUE(SunAboveHorizon(result.zenith_angle));
}

TEST(SolarPositionTest, Sunrise)
{
  // Sun near horizon (zenith ≈ 90°) at sunrise
  // Approximate sunrise at equator on equinox is 6:00 local = 6:00 UTC at lon 0
  auto result = CalculateSolarPosition(2024, 3, 20, 6.0, 0.0, 0.0);

  // Zenith should be close to 90° at sunrise
  EXPECT_NEAR(result.zenith_angle, 90.0, 5.0);
}

TEST(SolarPositionTest, Night)
{
  // Midnight at equator
  auto result = CalculateSolarPosition(2024, 6, 21, 0.0, 0.0, 0.0);

  // Sun should be below horizon
  EXPECT_GT(result.zenith_angle, 90.0);
  EXPECT_FALSE(SunAboveHorizon(result.zenith_angle));
}

TEST(SolarPositionTest, ArcticSummer)
{
  // Svalbard (78°N) on June 21 at midnight - should still have sun up
  auto result = CalculateSolarPosition(2024, 6, 21, 0.0, 78.0, 15.0);

  // Midnight sun - zenith angle should be < 90
  EXPECT_LT(result.zenith_angle, 90.0);
  EXPECT_TRUE(SunAboveHorizon(result.zenith_angle));
}

// ============================================================================
// Convenience Function Tests
// ============================================================================

TEST(SolarZenithAngleTest, MatchesFullCalculation)
{
  double sza = SolarZenithAngle(2024, 6, 21, 12.0, 0.0, 0.0);
  auto result = CalculateSolarPosition(2024, 6, 21, 12.0, 0.0, 0.0);

  EXPECT_DOUBLE_EQ(sza, result.zenith_angle);
}

TEST(CosineZenithAngleTest, KnownValues)
{
  EXPECT_NEAR(CosineZenithAngle(0.0), 1.0, 1e-10);
  EXPECT_NEAR(CosineZenithAngle(90.0), 0.0, 1e-10);
  EXPECT_NEAR(CosineZenithAngle(60.0), 0.5, 1e-10);
}

TEST(IsTwilightTest, DayTime)
{
  EXPECT_FALSE(IsTwilight(45.0));  // Day
}

TEST(IsTwilightTest, TwilightZone)
{
  EXPECT_TRUE(IsTwilight(95.0));   // Civil twilight
  EXPECT_TRUE(IsTwilight(100.0));  // Nautical twilight
  EXPECT_TRUE(IsTwilight(105.0));  // Astronomical twilight
}

TEST(IsTwilightTest, Night)
{
  EXPECT_FALSE(IsTwilight(110.0));  // Full night (beyond astronomical twilight)
}

TEST(SunAboveHorizonTest, BoundaryConditions)
{
  EXPECT_TRUE(SunAboveHorizon(89.9));
  EXPECT_FALSE(SunAboveHorizon(90.0));
  EXPECT_FALSE(SunAboveHorizon(90.1));
}

// ============================================================================
// Physical Consistency Tests
// ============================================================================

TEST(SolarPositionTest, ZenithAngleRange)
{
  // Test many random conditions - zenith should always be in [0, 180]
  for (int month = 1; month <= 12; ++month)
  {
    for (double hour = 0; hour <= 24; hour += 6)
    {
      auto result = CalculateSolarPosition(2024, month, 15, hour, 45.0, -90.0);
      EXPECT_GE(result.zenith_angle, 0.0);
      EXPECT_LE(result.zenith_angle, 180.0);
    }
  }
}

TEST(SolarPositionTest, SymmetryAroundNoon)
{
  // Zenith angle should be symmetric around solar noon
  double lat = 40.0;
  double lon = 0.0;

  // Solar noon at lon 0 is 12:00 UTC
  auto noon = CalculateSolarPosition(2024, 6, 21, 12.0, lat, lon);
  auto am = CalculateSolarPosition(2024, 6, 21, 10.0, lat, lon);
  auto pm = CalculateSolarPosition(2024, 6, 21, 14.0, lat, lon);

  // Zenith angles at equal times from noon should be approximately equal
  EXPECT_NEAR(am.zenith_angle, pm.zenith_angle, 1.0);
  // Noon should have minimum zenith
  EXPECT_LT(noon.zenith_angle, am.zenith_angle);
}

TEST(SolarPositionTest, LatitudeDependence)
{
  // At noon on equinox, zenith angle increases with latitude
  double sza_0 = SolarZenithAngle(2024, 3, 20, 12.0, 0.0, 0.0);
  double sza_30 = SolarZenithAngle(2024, 3, 20, 12.0, 30.0, 0.0);
  double sza_60 = SolarZenithAngle(2024, 3, 20, 12.0, 60.0, 0.0);

  EXPECT_LT(sza_0, sza_30);
  EXPECT_LT(sza_30, sza_60);
}
