#include <tuvx/radiator/radiator_state.hpp>

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx;

// ============================================================================
// RadiatorState Initialization Tests
// ============================================================================

TEST(RadiatorStateTest, DefaultConstruction)
{
  RadiatorState state;

  EXPECT_TRUE(state.Empty());
  EXPECT_EQ(state.NumberOfLayers(), 0u);
  EXPECT_EQ(state.NumberOfWavelengths(), 0u);
}

TEST(RadiatorStateTest, Initialize)
{
  RadiatorState state;
  state.Initialize(3, 5);

  EXPECT_FALSE(state.Empty());
  EXPECT_EQ(state.NumberOfLayers(), 3u);
  EXPECT_EQ(state.NumberOfWavelengths(), 5u);

  // All values should be zero
  for (std::size_t i = 0; i < 3; ++i)
  {
    for (std::size_t j = 0; j < 5; ++j)
    {
      EXPECT_DOUBLE_EQ(state.optical_depth[i][j], 0.0);
      EXPECT_DOUBLE_EQ(state.single_scattering_albedo[i][j], 0.0);
      EXPECT_DOUBLE_EQ(state.asymmetry_factor[i][j], 0.0);
    }
  }
}

TEST(RadiatorStateTest, InitializeOverwrites)
{
  RadiatorState state;
  state.Initialize(2, 3);
  state.optical_depth[0][0] = 1.0;

  state.Initialize(4, 6);

  EXPECT_EQ(state.NumberOfLayers(), 4u);
  EXPECT_EQ(state.NumberOfWavelengths(), 6u);
  EXPECT_DOUBLE_EQ(state.optical_depth[0][0], 0.0);
}

// ============================================================================
// RadiatorState Accumulation Tests
// ============================================================================

TEST(RadiatorStateTest, AccumulateOpticalDepths)
{
  RadiatorState state1;
  state1.Initialize(2, 3);
  state1.optical_depth[0][0] = 0.1;
  state1.optical_depth[0][1] = 0.2;
  state1.optical_depth[1][0] = 0.3;

  RadiatorState state2;
  state2.Initialize(2, 3);
  state2.optical_depth[0][0] = 0.4;
  state2.optical_depth[0][1] = 0.5;
  state2.optical_depth[1][0] = 0.6;

  state1.Accumulate(state2);

  EXPECT_NEAR(state1.optical_depth[0][0], 0.5, 1e-10);
  EXPECT_NEAR(state1.optical_depth[0][1], 0.7, 1e-10);
  EXPECT_NEAR(state1.optical_depth[1][0], 0.9, 1e-10);
}

TEST(RadiatorStateTest, AccumulateSingleScatteringAlbedo)
{
  // Two absorbers with different albedos
  RadiatorState state1;
  state1.Initialize(1, 1);
  state1.optical_depth[0][0] = 1.0;
  state1.single_scattering_albedo[0][0] = 0.8;  // 80% scattering

  RadiatorState state2;
  state2.Initialize(1, 1);
  state2.optical_depth[0][0] = 1.0;
  state2.single_scattering_albedo[0][0] = 0.4;  // 40% scattering

  state1.Accumulate(state2);

  // Combined: τ = 2.0, ω = (1.0*0.8 + 1.0*0.4) / 2.0 = 0.6
  EXPECT_NEAR(state1.optical_depth[0][0], 2.0, 1e-10);
  EXPECT_NEAR(state1.single_scattering_albedo[0][0], 0.6, 1e-10);
}

TEST(RadiatorStateTest, AccumulateAsymmetryFactor)
{
  // Two scatterers with different asymmetry factors
  RadiatorState state1;
  state1.Initialize(1, 1);
  state1.optical_depth[0][0] = 1.0;
  state1.single_scattering_albedo[0][0] = 1.0;  // Pure scatterer
  state1.asymmetry_factor[0][0] = 0.0;          // Isotropic (Rayleigh-like)

  RadiatorState state2;
  state2.Initialize(1, 1);
  state2.optical_depth[0][0] = 1.0;
  state2.single_scattering_albedo[0][0] = 1.0;  // Pure scatterer
  state2.asymmetry_factor[0][0] = 0.8;          // Forward scattering (aerosol-like)

  state1.Accumulate(state2);

  // g = (1.0*1.0*0.0 + 1.0*1.0*0.8) / (1.0*1.0 + 1.0*1.0) = 0.4
  EXPECT_NEAR(state1.asymmetry_factor[0][0], 0.4, 1e-10);
}

TEST(RadiatorStateTest, AccumulateWithPureAbsorber)
{
  // Scatterer + pure absorber
  RadiatorState scatterer;
  scatterer.Initialize(1, 1);
  scatterer.optical_depth[0][0] = 1.0;
  scatterer.single_scattering_albedo[0][0] = 1.0;
  scatterer.asymmetry_factor[0][0] = 0.7;

  RadiatorState absorber;
  absorber.Initialize(1, 1);
  absorber.optical_depth[0][0] = 1.0;
  absorber.single_scattering_albedo[0][0] = 0.0;  // Pure absorber
  absorber.asymmetry_factor[0][0] = 0.0;

  scatterer.Accumulate(absorber);

  // τ = 2.0
  // ω = (1.0*1.0 + 1.0*0.0) / 2.0 = 0.5
  // g = (1.0*1.0*0.7 + 1.0*0.0*0.0) / (1.0*1.0 + 1.0*0.0) = 0.7
  EXPECT_NEAR(scatterer.optical_depth[0][0], 2.0, 1e-10);
  EXPECT_NEAR(scatterer.single_scattering_albedo[0][0], 0.5, 1e-10);
  EXPECT_NEAR(scatterer.asymmetry_factor[0][0], 0.7, 1e-10);
}

TEST(RadiatorStateTest, AccumulateEmpty)
{
  RadiatorState state;
  state.Initialize(2, 3);
  state.optical_depth[0][0] = 0.5;

  RadiatorState empty;

  state.Accumulate(empty);

  // Should be unchanged
  EXPECT_EQ(state.NumberOfLayers(), 2u);
  EXPECT_NEAR(state.optical_depth[0][0], 0.5, 1e-10);
}

TEST(RadiatorStateTest, AccumulateIntoEmpty)
{
  RadiatorState empty;

  RadiatorState state;
  state.Initialize(2, 3);
  state.optical_depth[0][0] = 0.5;
  state.single_scattering_albedo[0][0] = 0.8;
  state.asymmetry_factor[0][0] = 0.3;

  empty.Accumulate(state);

  EXPECT_EQ(empty.NumberOfLayers(), 2u);
  EXPECT_NEAR(empty.optical_depth[0][0], 0.5, 1e-10);
  EXPECT_NEAR(empty.single_scattering_albedo[0][0], 0.8, 1e-10);
  EXPECT_NEAR(empty.asymmetry_factor[0][0], 0.3, 1e-10);
}

TEST(RadiatorStateTest, AccumulateDimensionMismatch)
{
  RadiatorState state1;
  state1.Initialize(2, 3);

  RadiatorState state2;
  state2.Initialize(2, 4);  // Different wavelength count

  EXPECT_THROW(state1.Accumulate(state2), std::runtime_error);
}

// ============================================================================
// RadiatorState Scale Tests
// ============================================================================

TEST(RadiatorStateTest, Scale)
{
  RadiatorState state;
  state.Initialize(2, 2);
  state.optical_depth[0][0] = 1.0;
  state.optical_depth[0][1] = 2.0;
  state.optical_depth[1][0] = 3.0;
  state.optical_depth[1][1] = 4.0;

  state.Scale(0.5);

  EXPECT_NEAR(state.optical_depth[0][0], 0.5, 1e-10);
  EXPECT_NEAR(state.optical_depth[0][1], 1.0, 1e-10);
  EXPECT_NEAR(state.optical_depth[1][0], 1.5, 1e-10);
  EXPECT_NEAR(state.optical_depth[1][1], 2.0, 1e-10);
}

TEST(RadiatorStateTest, ScaleEmpty)
{
  RadiatorState state;
  state.Scale(2.0);  // Should not crash

  EXPECT_TRUE(state.Empty());
}

// ============================================================================
// RadiatorState Total Optical Depth Tests
// ============================================================================

TEST(RadiatorStateTest, TotalOpticalDepth)
{
  RadiatorState state;
  state.Initialize(3, 2);

  // Layer 0: [0.1, 0.2]
  // Layer 1: [0.3, 0.4]
  // Layer 2: [0.5, 0.6]
  state.optical_depth[0][0] = 0.1;
  state.optical_depth[0][1] = 0.2;
  state.optical_depth[1][0] = 0.3;
  state.optical_depth[1][1] = 0.4;
  state.optical_depth[2][0] = 0.5;
  state.optical_depth[2][1] = 0.6;

  auto total = state.TotalOpticalDepth();

  ASSERT_EQ(total.size(), 2u);
  EXPECT_NEAR(total[0], 0.9, 1e-10);  // 0.1 + 0.3 + 0.5
  EXPECT_NEAR(total[1], 1.2, 1e-10);  // 0.2 + 0.4 + 0.6
}

TEST(RadiatorStateTest, TotalOpticalDepthEmpty)
{
  RadiatorState state;
  auto total = state.TotalOpticalDepth();

  EXPECT_TRUE(total.empty());
}

// ============================================================================
// RadiatorState Physical Scenario Tests
// ============================================================================

TEST(RadiatorStateTest, RayleighScatteringProperties)
{
  // Rayleigh scattering: pure scatterer with isotropic phase function
  RadiatorState rayleigh;
  rayleigh.Initialize(1, 1);
  rayleigh.optical_depth[0][0] = 0.1;
  rayleigh.single_scattering_albedo[0][0] = 1.0;  // Conservative scattering
  rayleigh.asymmetry_factor[0][0] = 0.0;          // Isotropic

  EXPECT_DOUBLE_EQ(rayleigh.single_scattering_albedo[0][0], 1.0);
  EXPECT_DOUBLE_EQ(rayleigh.asymmetry_factor[0][0], 0.0);
}

TEST(RadiatorStateTest, AerosolProperties)
{
  // Typical aerosol: moderate scattering with forward phase function
  RadiatorState aerosol;
  aerosol.Initialize(1, 1);
  aerosol.optical_depth[0][0] = 0.2;
  aerosol.single_scattering_albedo[0][0] = 0.95;  // Mostly scattering
  aerosol.asymmetry_factor[0][0] = 0.7;           // Forward scattering

  EXPECT_GT(aerosol.asymmetry_factor[0][0], 0.5);
  EXPECT_LT(aerosol.single_scattering_albedo[0][0], 1.0);
}

TEST(RadiatorStateTest, MolecularAbsorberProperties)
{
  // O3 absorption: pure absorber
  RadiatorState o3;
  o3.Initialize(1, 1);
  o3.optical_depth[0][0] = 0.5;
  o3.single_scattering_albedo[0][0] = 0.0;  // Pure absorption
  o3.asymmetry_factor[0][0] = 0.0;

  EXPECT_DOUBLE_EQ(o3.single_scattering_albedo[0][0], 0.0);
}

TEST(RadiatorStateTest, CombinedAtmosphere)
{
  // Combine Rayleigh + aerosol + O3
  RadiatorState atmosphere;
  atmosphere.Initialize(1, 1);

  RadiatorState rayleigh;
  rayleigh.Initialize(1, 1);
  rayleigh.optical_depth[0][0] = 0.1;
  rayleigh.single_scattering_albedo[0][0] = 1.0;
  rayleigh.asymmetry_factor[0][0] = 0.0;

  RadiatorState aerosol;
  aerosol.Initialize(1, 1);
  aerosol.optical_depth[0][0] = 0.2;
  aerosol.single_scattering_albedo[0][0] = 0.9;
  aerosol.asymmetry_factor[0][0] = 0.7;

  RadiatorState o3;
  o3.Initialize(1, 1);
  o3.optical_depth[0][0] = 0.3;
  o3.single_scattering_albedo[0][0] = 0.0;
  o3.asymmetry_factor[0][0] = 0.0;

  atmosphere.Accumulate(rayleigh);
  atmosphere.Accumulate(aerosol);
  atmosphere.Accumulate(o3);

  // Total τ = 0.1 + 0.2 + 0.3 = 0.6
  EXPECT_NEAR(atmosphere.optical_depth[0][0], 0.6, 1e-10);

  // ω < 1 because of O3 absorption
  EXPECT_LT(atmosphere.single_scattering_albedo[0][0], 1.0);
  EXPECT_GT(atmosphere.single_scattering_albedo[0][0], 0.0);

  // g > 0 because of aerosol forward scattering
  EXPECT_GT(atmosphere.asymmetry_factor[0][0], 0.0);
}
