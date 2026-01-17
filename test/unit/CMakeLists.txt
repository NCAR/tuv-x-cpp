# Utility tests
create_tuvx_test(test_constants util/test_constants.cpp)
create_tuvx_test(test_error util/test_error.cpp)
create_tuvx_test(test_array util/test_array.cpp)

# Grid tests
create_tuvx_test(test_grid grid/test_grid.cpp)
create_tuvx_test(test_mutable_grid grid/test_mutable_grid.cpp)
create_tuvx_test(test_grid_warehouse grid/test_grid_warehouse.cpp)

# Profile tests
create_tuvx_test(test_profile profile/test_profile.cpp)
create_tuvx_test(test_mutable_profile profile/test_mutable_profile.cpp)
create_tuvx_test(test_profile_warehouse profile/test_profile_warehouse.cpp)

# Interpolation tests
create_tuvx_test(test_linear_interpolator interpolation/test_linear_interpolator.cpp)
create_tuvx_test(test_conserving_interpolator interpolation/test_conserving_interpolator.cpp)

# Cross-section tests
create_tuvx_test(test_cross_section cross_section/test_cross_section.cpp)
create_tuvx_test(test_cross_section_warehouse cross_section/test_cross_section_warehouse.cpp)
create_tuvx_test(test_cross_section_o3 cross_section/test_cross_section_o3.cpp)

# Quantum yield tests
create_tuvx_test(test_quantum_yield quantum_yield/test_quantum_yield.cpp)
create_tuvx_test(test_quantum_yield_warehouse quantum_yield/test_quantum_yield_warehouse.cpp)

# Radiator tests
create_tuvx_test(test_radiator_state radiator/test_radiator_state.cpp)
create_tuvx_test(test_radiator radiator/test_radiator.cpp)
create_tuvx_test(test_radiator_warehouse radiator/test_radiator_warehouse.cpp)

# Radiation field tests
create_tuvx_test(test_radiation_field radiation_field/test_radiation_field.cpp)

# Solar tests
create_tuvx_test(test_solar_position solar/test_solar_position.cpp)
create_tuvx_test(test_extraterrestrial_flux solar/test_extraterrestrial_flux.cpp)

# Surface tests
create_tuvx_test(test_surface_albedo surface/test_surface_albedo.cpp)

# Spherical geometry tests
create_tuvx_test(test_spherical_geometry spherical_geometry/test_spherical_geometry.cpp)
