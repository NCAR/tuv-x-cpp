#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <tuvx/quantum_yield/quantum_yield.hpp>

namespace tuvx
{
  /// @brief O3 photolysis quantum yield for O(1D) production
  ///
  /// Implements the quantum yield for the photolysis reaction:
  /// O3 + hv -> O(1D) + O2(1Dg)
  ///
  /// This is the primary pathway for stratospheric ozone destruction and
  /// tropospheric OH radical production. The quantum yield depends on
  /// wavelength and temperature.
  ///
  /// At wavelengths < 310 nm (Hartley band), the quantum yield is near unity.
  /// In the Huggins bands (310-340 nm), the quantum yield decreases with
  /// wavelength and increases at lower temperatures.
  ///
  /// The complementary channel O3 -> O(3P) + O2(3Sigma) has quantum yield
  /// phi_O3P = 1 - phi_O1D.
  ///
  /// Reference:
  /// - JPL Publication 19-5 (2019): Chemical Kinetics and Photochemical Data
  /// - Matsumi et al. (2002), J. Geophys. Res., 107, 4024
  class O3O1DQuantumYield : public QuantumYield
  {
   public:
    /// @brief Construct with JPL-recommended parameterization
    O3O1DQuantumYield()
    {
      name_ = "O3->O(1D)+O2";
      reactant_ = "O3";
      products_ = "O(1D)+O2";
    }

    /// @brief Clone this quantum yield
    std::unique_ptr<QuantumYield> Clone() const override
    {
      return std::make_unique<O3O1DQuantumYield>();
    }

    /// @brief Calculate O3 -> O(1D) quantum yield
    ///
    /// Uses the JPL-19 recommendation:
    /// - Below 306 nm: phi = 0.90
    /// - 306-328 nm: Matsumi parameterization with T dependence
    /// - Above 328 nm: phi decreases to ~0.08 at 340 nm
    std::vector<double>
    Calculate(const Grid& wavelength_grid, double temperature, double /*air_density*/) const override
    {
      std::size_t n_wavelengths = wavelength_grid.Spec().n_cells;
      std::vector<double> result(n_wavelengths);

      auto wavelengths = wavelength_grid.Midpoints();

      // Clamp temperature to valid range
      double T = std::clamp(temperature, 200.0, 320.0);

      for (std::size_t i = 0; i < n_wavelengths; ++i)
      {
        double wl = wavelengths[i];
        result[i] = CalculateAtWavelength(wl, T);
      }

      return result;
    }

   private:
    /// @brief Calculate quantum yield at a single wavelength
    /// @param wavelength Wavelength in nm
    /// @param temperature Temperature in K
    /// @return Quantum yield [0-1]
    double CalculateAtWavelength(double wavelength, double temperature) const
    {
      // Outside absorption range
      if (wavelength < 175.0 || wavelength > 400.0)
      {
        return 0.0;
      }

      // Hartley band: high quantum yield
      if (wavelength < 306.0)
      {
        return 0.90;
      }

      // Huggins band transition region (306-328 nm)
      // Using simplified Matsumi parameterization
      if (wavelength <= 328.0)
      {
        // Temperature-dependent transition
        // At T=298K, transition from 0.90 at 306nm to ~0.45 at 328nm
        // Lower T increases quantum yield in this region

        double wl_frac = (wavelength - 306.0) / (328.0 - 306.0);

        // Base quantum yield at 298 K
        double phi_298 = 0.90 - 0.45 * wl_frac;

        // Temperature correction: increase by ~0.3%/K below 298 K
        double T_factor = 1.0 + 0.003 * (298.0 - temperature);
        T_factor = std::clamp(T_factor, 0.8, 1.3);

        double phi = phi_298 * T_factor;
        return std::clamp(phi, 0.0, 1.0);
      }

      // Long wavelength tail (328-400 nm)
      if (wavelength <= 340.0)
      {
        // Continued decrease to ~0.08 at 340 nm
        double wl_frac = (wavelength - 328.0) / (340.0 - 328.0);
        double phi_328 = 0.45;
        double phi_340 = 0.08;

        double phi = phi_328 - (phi_328 - phi_340) * wl_frac;

        // Weak temperature dependence
        double T_factor = 1.0 + 0.001 * (298.0 - temperature);
        phi *= std::clamp(T_factor, 0.9, 1.1);

        return std::clamp(phi, 0.0, 1.0);
      }

      // Very weak production above 340 nm
      if (wavelength <= 370.0)
      {
        double wl_frac = (wavelength - 340.0) / (370.0 - 340.0);
        double phi = 0.08 * (1.0 - wl_frac);
        return std::max(phi, 0.0);
      }

      // Negligible above 370 nm
      return 0.0;
    }
  };

  /// @brief O3 photolysis quantum yield for O(3P) production
  ///
  /// Implements the quantum yield for:
  /// O3 + hv -> O(3P) + O2(3Sigma-g)
  ///
  /// This is the complementary channel to O(1D) production.
  /// phi_O3P = 1 - phi_O1D within the O3 absorption bands.
  class O3O3PQuantumYield : public QuantumYield
  {
   public:
    O3O3PQuantumYield()
    {
      name_ = "O3->O(3P)+O2";
      reactant_ = "O3";
      products_ = "O(3P)+O2";
    }

    std::unique_ptr<QuantumYield> Clone() const override
    {
      return std::make_unique<O3O3PQuantumYield>();
    }

    std::vector<double>
    Calculate(const Grid& wavelength_grid, double temperature, double air_density) const override
    {
      // Get O(1D) quantum yields
      O3O1DQuantumYield o1d_qy;
      auto phi_o1d = o1d_qy.Calculate(wavelength_grid, temperature, air_density);

      // O(3P) is complementary channel
      std::vector<double> result(phi_o1d.size());
      for (std::size_t i = 0; i < phi_o1d.size(); ++i)
      {
        result[i] = 1.0 - phi_o1d[i];
      }

      return result;
    }
  };

}  // namespace tuvx
