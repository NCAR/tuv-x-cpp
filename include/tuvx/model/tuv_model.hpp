#pragma once

#include <memory>
#include <string>
#include <vector>

#include <tuvx/cross_section/cross_section.hpp>
#include <tuvx/cross_section/cross_section_warehouse.hpp>
#include <tuvx/grid/grid.hpp>
#include <tuvx/grid/grid_warehouse.hpp>
#include <tuvx/model/model_config.hpp>
#include <tuvx/model/model_output.hpp>
#include <tuvx/photolysis/photolysis_rate.hpp>
#include <tuvx/profile/profile.hpp>
#include <tuvx/profile/profile_warehouse.hpp>
#include <tuvx/quantum_yield/quantum_yield.hpp>
#include <tuvx/quantum_yield/quantum_yield_warehouse.hpp>
#include <tuvx/radiation_field/radiation_field.hpp>
#include <tuvx/radiator/radiator.hpp>
#include <tuvx/radiator/radiator_state.hpp>
#include <tuvx/radiator/radiator_warehouse.hpp>
#include <tuvx/radiator/types/from_cross_section.hpp>
#include <tuvx/radiator/types/rayleigh.hpp>
#include <tuvx/radiator/types/aerosol.hpp>
#include <tuvx/cross_section/types/o3.hpp>
#include <tuvx/cross_section/types/o2.hpp>
#include <tuvx/solar/extraterrestrial_flux.hpp>
#include <tuvx/solar/solar_position.hpp>
#include <tuvx/solver/delta_eddington.hpp>
#include <tuvx/solver/solver.hpp>
#include <tuvx/spherical_geometry/spherical_geometry.hpp>
#include <tuvx/surface/surface_albedo.hpp>
#include <tuvx/util/array.hpp>

namespace tuvx
{
  /// @brief Main TUV radiative transfer and photolysis model
  ///
  /// TuvModel orchestrates all components needed for UV/visible radiative
  /// transfer calculations and photolysis rate computation. It manages:
  /// - Wavelength and altitude grids
  /// - Atmospheric profiles (T, p, density, O3)
  /// - Cross-sections and quantum yields
  /// - Radiative transfer solver
  /// - Photolysis rate calculations
  ///
  /// Example usage:
  /// @code
  /// ModelConfig config;
  /// config.solar_zenith_angle = 30.0;
  /// config.day_of_year = 172;  // Summer solstice
  ///
  /// TuvModel model(config);
  /// model.AddPhotolysisReaction("O3 -> O2 + O(1D)", &o3_xs, &o3_qy);
  ///
  /// auto output = model.Calculate();
  /// double j_o3 = output.GetSurfacePhotolysisRate("O3 -> O2 + O(1D)");
  /// @endcode
  class TuvModel
  {
   public:
    // ========================================================================
    // Construction
    // ========================================================================

    /// @brief Construct model with configuration
    explicit TuvModel(const ModelConfig& config = ModelConfig{})
        : config_(config)
    {
      Initialize();
    }

    /// @brief Construct model with configuration (move)
    explicit TuvModel(ModelConfig&& config)
        : config_(std::move(config))
    {
      Initialize();
    }

    // ========================================================================
    // Configuration
    // ========================================================================

    /// @brief Get current configuration
    const ModelConfig& Config() const
    {
      return config_;
    }

    /// @brief Update configuration
    /// @note This will reinitialize grids if they change
    void SetConfig(const ModelConfig& config)
    {
      config_ = config;
      Initialize();
    }

    // ========================================================================
    // Grid Setup
    // ========================================================================

    /// @brief Set custom wavelength grid
    /// @param edges Wavelength bin edges [nm]
    /// @return Reference to this model for chaining
    TuvModel& SetWavelengthGrid(std::vector<double> edges)
    {
      config_.wavelength_edges = std::move(edges);
      InitializeWavelengthGrid();
      return *this;
    }

    /// @brief Set custom altitude grid
    /// @param edges Altitude level edges [km]
    /// @return Reference to this model for chaining
    TuvModel& SetAltitudeGrid(std::vector<double> edges)
    {
      config_.altitude_edges = std::move(edges);
      InitializeAltitudeGrid();
      return *this;
    }

    // ========================================================================
    // Atmospheric Profile Setup
    // ========================================================================

    /// @brief Set temperature profile
    /// @param values Temperature at each layer midpoint [K]
    /// @return Reference to this model for chaining
    TuvModel& SetTemperatureProfile(std::vector<double> values)
    {
      config_.temperature_profile = std::move(values);
      return *this;
    }

    /// @brief Set pressure profile
    /// @param values Pressure at each layer midpoint [hPa]
    /// @return Reference to this model for chaining
    TuvModel& SetPressureProfile(std::vector<double> values)
    {
      config_.pressure_profile = std::move(values);
      return *this;
    }

    /// @brief Set air density profile
    /// @param values Air density at each layer midpoint [molecules/cm³]
    /// @return Reference to this model for chaining
    TuvModel& SetAirDensityProfile(std::vector<double> values)
    {
      config_.air_density_profile = std::move(values);
      return *this;
    }

    /// @brief Set ozone profile
    /// @param values Ozone column density per layer [molecules/cm²]
    /// @return Reference to this model for chaining
    TuvModel& SetOzoneProfile(std::vector<double> values)
    {
      config_.ozone_profile = std::move(values);
      return *this;
    }

    /// @brief Use US Standard Atmosphere 1976 for atmospheric profiles
    /// @return Reference to this model for chaining
    TuvModel& UseStandardAtmosphere()
    {
      auto midpoints_span = altitude_grid_.Midpoints();
      std::vector<double> midpoints(midpoints_span.begin(), midpoints_span.end());
      config_.temperature_profile = StandardAtmosphere::GenerateTemperatureProfile(midpoints);
      config_.pressure_profile = StandardAtmosphere::GeneratePressureProfile(midpoints);
      config_.air_density_profile = StandardAtmosphere::GenerateAirDensityProfile(midpoints);
      config_.ozone_profile = StandardAtmosphere::GenerateOzoneProfile(midpoints, config_.ozone_column_DU);
      return *this;
    }

    // ========================================================================
    // Surface Properties
    // ========================================================================

    /// @brief Set uniform surface albedo
    /// @param albedo Surface albedo [0-1]
    /// @return Reference to this model for chaining
    TuvModel& SetSurfaceAlbedo(double albedo)
    {
      config_.surface_albedo = albedo;
      config_.surface_albedo_spectrum.clear();
      return *this;
    }

    /// @brief Set wavelength-dependent surface albedo
    /// @param albedo_spectrum Albedo at each wavelength bin [0-1]
    /// @return Reference to this model for chaining
    TuvModel& SetSurfaceAlbedoSpectrum(std::vector<double> albedo_spectrum)
    {
      config_.surface_albedo_spectrum = std::move(albedo_spectrum);
      return *this;
    }

    // ========================================================================
    // Radiator Setup
    // ========================================================================

    /// @brief Add a radiator (absorber/scatterer) to the model
    /// @param radiator Radiator to add (model takes ownership via clone)
    /// @return Reference to this model for chaining
    TuvModel& AddRadiator(const Radiator& radiator)
    {
      radiators_.Add(radiator.Clone());
      return *this;
    }

    /// @brief Add ozone (O3) as a radiator using default cross-sections
    ///
    /// This convenience method creates an O3 radiator using the JPL-19
    /// recommended cross-sections with temperature dependence. The O3
    /// number density profile is taken from config_.ozone_profile or
    /// generated from StandardAtmosphere using config_.ozone_column_DU.
    ///
    /// @return Reference to this model for chaining
    TuvModel& AddO3Radiator()
    {
      auto o3_radiator = std::make_unique<FromCrossSectionRadiator>(
          "O3",
          std::make_unique<O3CrossSection>(),
          "O3",          // density profile name
          "temperature", // temperature profile name
          "wavelength",  // wavelength grid name
          "altitude"     // altitude grid name
      );
      radiators_.Add(std::move(o3_radiator));
      return *this;
    }

    /// @brief Add oxygen (O2) as a radiator using Schumann-Runge cross-sections
    ///
    /// This convenience method creates an O2 radiator using simplified
    /// Schumann-Runge band cross-sections. The O2 number density profile
    /// is calculated as 20.95% of the air density profile.
    ///
    /// @return Reference to this model for chaining
    TuvModel& AddO2Radiator()
    {
      auto o2_radiator = std::make_unique<FromCrossSectionRadiator>(
          "O2",
          std::make_unique<O2CrossSection>(),
          "O2",          // density profile name
          "temperature", // temperature profile name
          "wavelength",  // wavelength grid name
          "altitude"     // altitude grid name
      );
      radiators_.Add(std::move(o2_radiator));
      return *this;
    }

    /// @brief Add Rayleigh (molecular) scattering
    ///
    /// This adds molecular scattering based on air density. Rayleigh
    /// scattering has ω = 1 (pure scattering) and g = 0 (isotropic).
    /// The cross-section follows λ^-4 wavelength dependence.
    ///
    /// @return Reference to this model for chaining
    TuvModel& AddRayleighRadiator()
    {
      auto rayleigh_radiator = std::make_unique<RayleighRadiator>(
          "air_density", // air density profile name
          "wavelength",  // wavelength grid name
          "altitude"     // altitude grid name
      );
      radiators_.Add(std::move(rayleigh_radiator));
      return *this;
    }

    /// @brief Add aerosol radiator with default configuration
    ///
    /// Default configuration: τ = 0.1 at 550 nm, Ångström exponent = 1.3,
    /// scale height = 2 km, ω = 0.9, g = 0.7.
    ///
    /// @return Reference to this model for chaining
    TuvModel& AddAerosolRadiator()
    {
      auto aerosol_radiator = std::make_unique<AerosolRadiator>();
      radiators_.Add(std::move(aerosol_radiator));
      return *this;
    }

    /// @brief Add aerosol radiator with custom configuration
    ///
    /// @param config Aerosol configuration parameters
    /// @return Reference to this model for chaining
    TuvModel& AddAerosolRadiator(AerosolRadiator::Config config)
    {
      auto aerosol_radiator = std::make_unique<AerosolRadiator>(std::move(config));
      radiators_.Add(std::move(aerosol_radiator));
      return *this;
    }

    /// @brief Add all standard radiators (O3, O2, Rayleigh)
    ///
    /// Convenience method to add the major atmospheric absorbers and
    /// scatterers in one call. Does not include aerosols (use AddAerosolRadiator
    /// separately if needed).
    ///
    /// @return Reference to this model for chaining
    TuvModel& AddStandardRadiators()
    {
      AddO3Radiator();
      AddO2Radiator();
      AddRayleighRadiator();
      return *this;
    }

    // ========================================================================
    // Photolysis Reaction Setup
    // ========================================================================

    /// @brief Add a photolysis reaction
    /// @param name Reaction name (e.g., "O3 -> O2 + O(1D)")
    /// @param cross_section Absorption cross-section (must remain valid)
    /// @param quantum_yield Quantum yield (must remain valid)
    /// @return Reference to this model for chaining
    TuvModel& AddPhotolysisReaction(
        const std::string& name,
        const CrossSection* cross_section,
        const QuantumYield* quantum_yield)
    {
      photolysis_reactions_.AddReaction(name, cross_section, quantum_yield);
      return *this;
    }

    // ========================================================================
    // Calculation
    // ========================================================================

    /// @brief Calculate radiation field and photolysis rates
    /// @return Model output containing all computed quantities
    ModelOutput Calculate()
    {
      return Calculate(config_.solar_zenith_angle);
    }

    /// @brief Calculate for a specific solar zenith angle
    /// @param solar_zenith_angle Solar zenith angle [degrees]
    /// @return Model output
    ModelOutput Calculate(double solar_zenith_angle)
    {
      ModelOutput output;

      // Store calculation metadata
      output.solar_zenith_angle = solar_zenith_angle;
      output.day_of_year = config_.day_of_year;
      output.earth_sun_distance = config_.EffectiveEarthSunDistance();
      output.is_daytime = solar_zenith_angle < 90.0;
      output.used_spherical_geometry = config_.use_spherical_geometry;

      // Store grids
      output.wavelength_grid = wavelength_grid_;
      output.altitude_grid = altitude_grid_;

      // Get number of levels and wavelengths
      std::size_t n_layers = altitude_grid_.Spec().n_cells;
      std::size_t n_wavelengths = wavelength_grid_.Spec().n_cells;

      // Compute spherical geometry if enabled
      SphericalGeometry::SlantPathResult geometry;
      if (config_.use_spherical_geometry)
      {
        SphericalGeometry spherical_geom(altitude_grid_, config_.earth_radius);
        geometry = spherical_geom.Calculate(solar_zenith_angle);
      }

      // Get extraterrestrial flux using ASTM E-490 reference spectrum
      auto et_flux = solar::reference_spectra::CreateASTM_E490();
      auto solar_flux = et_flux.Calculate(wavelength_grid_);

      // Apply Earth-Sun distance correction
      double distance_factor = 1.0 / (output.earth_sun_distance * output.earth_sun_distance);
      for (auto& flux : solar_flux)
      {
        flux *= distance_factor;
      }

      // Get surface albedo
      std::vector<double> surface_albedo;
      if (!config_.surface_albedo_spectrum.empty())
      {
        surface_albedo = config_.surface_albedo_spectrum;
      }
      else
      {
        surface_albedo.assign(n_wavelengths, config_.surface_albedo);
      }

      // Get altitude midpoints for profile generation
      auto midpoints_span = altitude_grid_.Midpoints();
      std::vector<double> midpoints_vec(midpoints_span.begin(), midpoints_span.end());

      // Get temperature profile for cross-section calculations
      std::vector<double> temperatures = config_.temperature_profile;
      if (temperatures.empty())
      {
        temperatures = StandardAtmosphere::GenerateTemperatureProfile(midpoints_vec);
      }

      // Get air density profile
      std::vector<double> air_density = config_.air_density_profile;
      if (air_density.empty())
      {
        air_density = StandardAtmosphere::GenerateAirDensityProfile(midpoints_vec);
      }

      // Get ozone profile
      std::vector<double> ozone = config_.ozone_profile;
      if (ozone.empty())
      {
        ozone = StandardAtmosphere::GenerateOzoneProfile(midpoints_vec, config_.ozone_column_DU);
      }

      // Get O2 profile (20.95% of air density)
      std::vector<double> o2;
      o2.reserve(air_density.size());
      for (double air_n : air_density)
      {
        o2.push_back(air_n * StandardAtmosphere::kO2MixingRatio);
      }

      // Combine radiator states
      RadiatorState combined_state;
      combined_state.Initialize(n_layers, n_wavelengths);

      // Update radiators if any are configured
      if (!radiators_.Empty())
      {
        // Create grid warehouse
        GridWarehouse grids;
        grids.Add(wavelength_grid_);
        grids.Add(altitude_grid_);

        // Create profile warehouse with atmospheric profiles
        ProfileWarehouse profiles;
        profiles.Add(Profile(
            ProfileSpec{ "temperature", "K", n_layers },
            temperatures));
        profiles.Add(Profile(
            ProfileSpec{ "air_density", "molecules/cm^3", n_layers },
            air_density));
        profiles.Add(Profile(
            ProfileSpec{ "O3", "molecules/cm^3", n_layers },
            ozone));
        profiles.Add(Profile(
            ProfileSpec{ "O2", "molecules/cm^3", n_layers },
            o2));

        // Update all radiators with current atmospheric state
        radiators_.UpdateAll(grids, profiles);

        // Get combined optical properties from all radiators
        combined_state = radiators_.CombinedState();
      }

      // Set up solver input
      SolverInput solver_input;
      solver_input.radiator_state = &combined_state;
      solver_input.solar_zenith_angle = solar_zenith_angle;
      solver_input.extraterrestrial_flux = &solar_flux;
      solver_input.surface_albedo = &surface_albedo;

      if (config_.use_spherical_geometry)
      {
        solver_input.geometry = &geometry;
      }

      // Solve radiative transfer
      output.radiation_field = solver_->Solve(solver_input);

      // Calculate photolysis rates
      output.photolysis_rates = photolysis_reactions_.CalculateAll(
          output.radiation_field, wavelength_grid_, temperatures);

      return output;
    }

    /// @brief Calculate for a specific location and time
    /// @param year Year
    /// @param month Month [1-12]
    /// @param day Day of month [1-31]
    /// @param hour Hour (UTC) with fractional hours
    /// @param latitude Latitude [degrees]
    /// @param longitude Longitude [degrees]
    /// @return Model output
    ModelOutput Calculate(
        int year,
        int month,
        int day,
        double hour,
        double latitude,
        double longitude)
    {
      // Calculate solar position using free functions from solar namespace
      auto position = solar::CalculateSolarPosition(year, month, day, hour, latitude, longitude);

      // Update config
      config_.solar_zenith_angle = position.zenith_angle;
      config_.day_of_year = solar::DayOfYear(year, month, day);
      config_.latitude = latitude;
      config_.longitude = longitude;

      return Calculate(position.zenith_angle);
    }

    // ========================================================================
    // Access to Internal Components
    // ========================================================================

    /// @brief Get wavelength grid
    const Grid& WavelengthGrid() const
    {
      return wavelength_grid_;
    }

    /// @brief Get altitude grid
    const Grid& AltitudeGrid() const
    {
      return altitude_grid_;
    }

    /// @brief Get radiator warehouse
    const RadiatorWarehouse& Radiators() const
    {
      return radiators_;
    }

    /// @brief Get photolysis reaction set
    const PhotolysisRateSet& PhotolysisReactions() const
    {
      return photolysis_reactions_;
    }

    /// @brief Get grid warehouse containing wavelength and altitude grids
    /// @return GridWarehouse with model grids
    GridWarehouse GetGridWarehouse() const
    {
      GridWarehouse grids;
      grids.Add(wavelength_grid_);
      grids.Add(altitude_grid_);
      return grids;
    }

    /// @brief Create profile warehouse with current atmospheric profiles
    /// @return ProfileWarehouse with temperature, air density, O3, O2 profiles
    ProfileWarehouse CreateProfileWarehouse() const
    {
      std::size_t n_layers = altitude_grid_.Spec().n_cells;

      // Get altitude midpoints for profile generation
      auto midpoints_span = altitude_grid_.Midpoints();
      std::vector<double> midpoints_vec(midpoints_span.begin(), midpoints_span.end());

      // Get temperature profile
      std::vector<double> temperatures = config_.temperature_profile;
      if (temperatures.empty())
      {
        temperatures = StandardAtmosphere::GenerateTemperatureProfile(midpoints_vec);
      }

      // Get air density profile
      std::vector<double> air_density = config_.air_density_profile;
      if (air_density.empty())
      {
        air_density = StandardAtmosphere::GenerateAirDensityProfile(midpoints_vec);
      }

      // Get ozone profile
      std::vector<double> ozone = config_.ozone_profile;
      if (ozone.empty())
      {
        ozone = StandardAtmosphere::GenerateOzoneProfile(midpoints_vec, config_.ozone_column_DU);
      }

      // Get O2 profile (20.95% of air density)
      std::vector<double> o2;
      o2.reserve(air_density.size());
      for (double air_n : air_density)
      {
        o2.push_back(air_n * StandardAtmosphere::kO2MixingRatio);
      }

      // Create profile warehouse
      ProfileWarehouse profiles;
      profiles.Add(Profile(
          ProfileSpec{ "temperature", "K", n_layers },
          temperatures));
      profiles.Add(Profile(
          ProfileSpec{ "air_density", "molecules/cm^3", n_layers },
          air_density));
      profiles.Add(Profile(
          ProfileSpec{ "O3", "molecules/cm^3", n_layers },
          ozone));
      profiles.Add(Profile(
          ProfileSpec{ "O2", "molecules/cm^3", n_layers },
          o2));

      return profiles;
    }

   private:
    /// @brief Initialize model components from configuration
    void Initialize()
    {
      InitializeWavelengthGrid();
      InitializeAltitudeGrid();
      InitializeSolver();
    }

    /// @brief Initialize wavelength grid
    void InitializeWavelengthGrid()
    {
      if (!config_.wavelength_edges.empty())
      {
        GridSpec spec{ "wavelength", "nm", config_.wavelength_edges.size() - 1 };
        wavelength_grid_ = Grid(spec, config_.wavelength_edges);
      }
      else
      {
        // Create default equally-spaced grid
        GridSpec spec{ "wavelength", "nm", config_.n_wavelength_bins };
        wavelength_grid_ = Grid::EquallySpaced(spec, config_.wavelength_min, config_.wavelength_max);
      }
    }

    /// @brief Initialize altitude grid
    void InitializeAltitudeGrid()
    {
      if (!config_.altitude_edges.empty())
      {
        GridSpec spec{ "altitude", "km", config_.altitude_edges.size() - 1 };
        altitude_grid_ = Grid(spec, config_.altitude_edges);
      }
      else
      {
        // Create default equally-spaced grid
        GridSpec spec{ "altitude", "km", config_.n_altitude_layers };
        altitude_grid_ = Grid::EquallySpaced(spec, config_.altitude_min, config_.altitude_max);
      }
    }

    /// @brief Initialize solver
    void InitializeSolver()
    {
      // Currently only Delta-Eddington is supported
      solver_ = std::make_unique<DeltaEddingtonSolver>();
    }

    // Configuration
    ModelConfig config_;

    // Grids
    Grid wavelength_grid_;
    Grid altitude_grid_;

    // Component warehouses
    RadiatorWarehouse radiators_;

    // Photolysis reactions
    PhotolysisRateSet photolysis_reactions_;

    // Solver
    std::unique_ptr<Solver> solver_;
  };

}  // namespace tuvx
