#include <tuvx/radiation_field/radiation_field.hpp>

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// ============================================================================
// RadiationField Initialization Tests
// ============================================================================

TEST(RadiationFieldTest, DefaultConstruction)
{
  RadiationField field;

  EXPECT_TRUE(field.Empty());
  EXPECT_EQ(field.NumberOfLevels(), 0u);
  EXPECT_EQ(field.NumberOfWavelengths(), 0u);
}

TEST(RadiationFieldTest, Initialize)
{
  RadiationField field;
  field.Initialize(4, 5);  // 4 levels (3 layers), 5 wavelengths

  EXPECT_FALSE(field.Empty());
  EXPECT_EQ(field.NumberOfLevels(), 4u);
  EXPECT_EQ(field.NumberOfWavelengths(), 5u);

  // All values should be zero
  for (std::size_t i = 0; i < 4; ++i)
  {
    for (std::size_t j = 0; j < 5; ++j)
    {
      EXPECT_DOUBLE_EQ(field.direct_irradiance[i][j], 0.0);
      EXPECT_DOUBLE_EQ(field.diffuse_up[i][j], 0.0);
      EXPECT_DOUBLE_EQ(field.diffuse_down[i][j], 0.0);
      EXPECT_DOUBLE_EQ(field.actinic_flux_direct[i][j], 0.0);
      EXPECT_DOUBLE_EQ(field.actinic_flux_diffuse[i][j], 0.0);
    }
  }
}

TEST(RadiationFieldTest, InitializeOverwrites)
{
  RadiationField field;
  field.Initialize(2, 3);
  field.direct_irradiance[0][0] = 100.0;

  field.Initialize(4, 5);

  EXPECT_EQ(field.NumberOfLevels(), 4u);
  EXPECT_EQ(field.NumberOfWavelengths(), 5u);
  EXPECT_DOUBLE_EQ(field.direct_irradiance[0][0], 0.0);
}

// ============================================================================
// RadiationField Total Actinic Flux Tests
// ============================================================================

TEST(RadiationFieldTest, TotalActinicFlux)
{
  RadiationField field;
  field.Initialize(3, 2);

  // Set some values at level 1
  field.actinic_flux_direct[1][0] = 100.0;
  field.actinic_flux_direct[1][1] = 200.0;
  field.actinic_flux_diffuse[1][0] = 50.0;
  field.actinic_flux_diffuse[1][1] = 80.0;

  auto total = field.TotalActinicFlux(1);

  ASSERT_EQ(total.size(), 2u);
  EXPECT_NEAR(total[0], 150.0, 1e-10);
  EXPECT_NEAR(total[1], 280.0, 1e-10);
}

TEST(RadiationFieldTest, TotalActinicFluxInvalidLevel)
{
  RadiationField field;
  field.Initialize(3, 2);

  auto total = field.TotalActinicFlux(10);  // Invalid level

  EXPECT_TRUE(total.empty());
}

TEST(RadiationFieldTest, SurfaceActinicFlux)
{
  RadiationField field;
  field.Initialize(3, 2);

  field.actinic_flux_direct[0][0] = 100.0;
  field.actinic_flux_diffuse[0][0] = 50.0;

  auto surface = field.SurfaceActinicFlux();

  ASSERT_EQ(surface.size(), 2u);
  EXPECT_NEAR(surface[0], 150.0, 1e-10);
}

// ============================================================================
// RadiationField Total Downwelling Tests
// ============================================================================

TEST(RadiationFieldTest, TotalDownwelling)
{
  RadiationField field;
  field.Initialize(3, 2);

  field.direct_irradiance[0][0] = 800.0;
  field.diffuse_down[0][0] = 200.0;

  auto total = field.TotalDownwelling(0);

  ASSERT_EQ(total.size(), 2u);
  EXPECT_NEAR(total[0], 1000.0, 1e-10);
}

TEST(RadiationFieldTest, SurfaceGlobalIrradiance)
{
  RadiationField field;
  field.Initialize(3, 2);

  field.direct_irradiance[0][0] = 700.0;
  field.direct_irradiance[0][1] = 600.0;
  field.diffuse_down[0][0] = 300.0;
  field.diffuse_down[0][1] = 400.0;

  auto global = field.SurfaceGlobalIrradiance();

  ASSERT_EQ(global.size(), 2u);
  EXPECT_NEAR(global[0], 1000.0, 1e-10);
  EXPECT_NEAR(global[1], 1000.0, 1e-10);
}

// ============================================================================
// RadiationField Scale Tests
// ============================================================================

TEST(RadiationFieldTest, Scale)
{
  RadiationField field;
  field.Initialize(2, 2);

  field.direct_irradiance[0][0] = 100.0;
  field.diffuse_up[0][0] = 50.0;
  field.diffuse_down[0][0] = 30.0;
  field.actinic_flux_direct[0][0] = 200.0;
  field.actinic_flux_diffuse[0][0] = 80.0;

  field.Scale(0.5);

  EXPECT_NEAR(field.direct_irradiance[0][0], 50.0, 1e-10);
  EXPECT_NEAR(field.diffuse_up[0][0], 25.0, 1e-10);
  EXPECT_NEAR(field.diffuse_down[0][0], 15.0, 1e-10);
  EXPECT_NEAR(field.actinic_flux_direct[0][0], 100.0, 1e-10);
  EXPECT_NEAR(field.actinic_flux_diffuse[0][0], 40.0, 1e-10);
}

TEST(RadiationFieldTest, ScaleEmpty)
{
  RadiationField field;
  field.Scale(2.0);  // Should not crash

  EXPECT_TRUE(field.Empty());
}

// ============================================================================
// RadiationField Accumulate Tests
// ============================================================================

TEST(RadiationFieldTest, Accumulate)
{
  RadiationField field1;
  field1.Initialize(2, 2);
  field1.direct_irradiance[0][0] = 100.0;
  field1.actinic_flux_direct[0][0] = 200.0;

  RadiationField field2;
  field2.Initialize(2, 2);
  field2.direct_irradiance[0][0] = 50.0;
  field2.actinic_flux_direct[0][0] = 80.0;

  field1.Accumulate(field2);

  EXPECT_NEAR(field1.direct_irradiance[0][0], 150.0, 1e-10);
  EXPECT_NEAR(field1.actinic_flux_direct[0][0], 280.0, 1e-10);
}

TEST(RadiationFieldTest, AccumulateEmpty)
{
  RadiationField field;
  field.Initialize(2, 2);
  field.direct_irradiance[0][0] = 100.0;

  RadiationField empty;
  field.Accumulate(empty);

  // Should be unchanged
  EXPECT_NEAR(field.direct_irradiance[0][0], 100.0, 1e-10);
}

TEST(RadiationFieldTest, AccumulateIntoEmpty)
{
  RadiationField empty;

  RadiationField field;
  field.Initialize(2, 2);
  field.direct_irradiance[0][0] = 100.0;

  empty.Accumulate(field);

  EXPECT_EQ(empty.NumberOfLevels(), 2u);
  EXPECT_NEAR(empty.direct_irradiance[0][0], 100.0, 1e-10);
}

// ============================================================================
// RadiationField Physical Scenario Tests
// ============================================================================

TEST(RadiationFieldTest, ClearSkyProfile)
{
  // Simulate a clear sky profile with decreasing direct and increasing diffuse
  RadiationField field;
  field.Initialize(4, 1);  // 4 levels, 1 wavelength

  // TOA: all direct, no diffuse
  field.direct_irradiance[3][0] = 1000.0;
  field.actinic_flux_direct[3][0] = 1200.0;

  // Mid-atmosphere: some attenuation
  field.direct_irradiance[2][0] = 800.0;
  field.diffuse_down[2][0] = 50.0;
  field.actinic_flux_direct[2][0] = 960.0;
  field.actinic_flux_diffuse[2][0] = 100.0;

  // Lower atmosphere: more attenuation
  field.direct_irradiance[1][0] = 600.0;
  field.diffuse_down[1][0] = 150.0;
  field.diffuse_up[1][0] = 50.0;
  field.actinic_flux_direct[1][0] = 720.0;
  field.actinic_flux_diffuse[1][0] = 300.0;

  // Surface: significant diffuse component
  field.direct_irradiance[0][0] = 500.0;
  field.diffuse_down[0][0] = 200.0;
  field.diffuse_up[0][0] = 100.0;  // Surface reflection
  field.actinic_flux_direct[0][0] = 600.0;
  field.actinic_flux_diffuse[0][0] = 400.0;

  // Direct should decrease with depth
  EXPECT_GT(field.direct_irradiance[3][0], field.direct_irradiance[0][0]);

  // Diffuse down should be significant at surface
  EXPECT_GT(field.diffuse_down[0][0], 0.0);

  // Total surface irradiance
  auto global = field.SurfaceGlobalIrradiance();
  EXPECT_NEAR(global[0], 700.0, 1e-10);

  // Total surface actinic flux
  auto actinic = field.SurfaceActinicFlux();
  EXPECT_NEAR(actinic[0], 1000.0, 1e-10);
}

TEST(RadiationFieldTest, TimeAveragingExample)
{
  // Simulate averaging over multiple time steps
  RadiationField morning;
  morning.Initialize(2, 1);
  morning.direct_irradiance[0][0] = 500.0;

  RadiationField noon;
  noon.Initialize(2, 1);
  noon.direct_irradiance[0][0] = 1000.0;

  RadiationField afternoon;
  afternoon.Initialize(2, 1);
  afternoon.direct_irradiance[0][0] = 600.0;

  // Accumulate with equal weights
  RadiationField daily;
  daily.Accumulate(morning);
  daily.Accumulate(noon);
  daily.Accumulate(afternoon);
  daily.Scale(1.0 / 3.0);

  EXPECT_NEAR(daily.direct_irradiance[0][0], 700.0, 1e-10);
}
