#pragma once

// TUV-x C++ Library - Main convenience header
// Include this header to access all TUV-x functionality

// Utility headers
#include <tuvx/util/array.hpp>
#include <tuvx/util/constants.hpp>
#include <tuvx/util/error.hpp>
#include <tuvx/util/internal_error.hpp>

// Grid system headers
#include <tuvx/grid/grid_spec.hpp>
#include <tuvx/grid/grid.hpp>
#include <tuvx/grid/mutable_grid.hpp>
#include <tuvx/grid/grid_warehouse.hpp>

// Profile system headers
#include <tuvx/profile/profile_spec.hpp>
#include <tuvx/profile/profile.hpp>
#include <tuvx/profile/mutable_profile.hpp>
#include <tuvx/profile/profile_warehouse.hpp>

// Interpolation headers
#include <tuvx/interpolation/interpolator.hpp>
#include <tuvx/interpolation/linear_interpolator.hpp>
#include <tuvx/interpolation/conserving_interpolator.hpp>

// Cross-section headers
#include <tuvx/cross_section/cross_section.hpp>
#include <tuvx/cross_section/temperature_based.hpp>
#include <tuvx/cross_section/cross_section_warehouse.hpp>
#include <tuvx/cross_section/types/base.hpp>
#include <tuvx/cross_section/types/o3.hpp>

// Quantum yield headers
#include <tuvx/quantum_yield/quantum_yield.hpp>
#include <tuvx/quantum_yield/quantum_yield_warehouse.hpp>
#include <tuvx/quantum_yield/types/base.hpp>
#include <tuvx/quantum_yield/types/o3_o1d.hpp>

// Radiator headers
#include <tuvx/radiator/radiator_state.hpp>
#include <tuvx/radiator/radiator.hpp>
#include <tuvx/radiator/radiator_warehouse.hpp>
#include <tuvx/radiator/types/from_cross_section.hpp>

// Radiation field headers
#include <tuvx/radiation_field/radiation_field.hpp>

// Solar position and flux headers
#include <tuvx/solar/solar_position.hpp>
#include <tuvx/solar/extraterrestrial_flux.hpp>

// Surface albedo headers
#include <tuvx/surface/surface_albedo.hpp>

// Spherical geometry headers
#include <tuvx/spherical_geometry/spherical_geometry.hpp>

// Solver headers
#include <tuvx/solver/solver.hpp>
#include <tuvx/solver/delta_eddington.hpp>

// Photolysis rate headers
#include <tuvx/photolysis/photolysis_rate.hpp>

// Version information
#include <tuvx/version.hpp>
