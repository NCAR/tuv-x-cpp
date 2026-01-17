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
