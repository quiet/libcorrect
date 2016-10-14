add_executable(rs_find_primitive_poly EXCLUDE_FROM_ALL find_rs_primitive_poly.c)
target_link_libraries(rs_find_primitive_poly correct_static)

add_executable(conv_find_libfec_poly EXCLUDE_FROM_ALL find_conv_libfec_poly.c)
target_link_libraries(conv_find_libfec_poly correct_static fec)

if(HAVE_SSE)
    add_executable(conv_find_optim_poly EXCLUDE_FROM_ALL find_conv_optim_poly.c $<TARGET_OBJECTS:error_sim_sse>)
    target_link_libraries(conv_find_optim_poly correct_static)

    add_executable(conv_find_optim_poly_annealing EXCLUDE_FROM_ALL find_conv_optim_poly_annealing.c $<TARGET_OBJECTS:error_sim_sse>)
    target_link_libraries(conv_find_optim_poly_annealing correct_static)
else()
    add_executable(conv_find_optim_poly EXCLUDE_FROM_ALL find_conv_optim_poly.c $<TARGET_OBJECTS:error_sim>)
    target_link_libraries(conv_find_optim_poly correct_static)

    add_executable(conv_find_optim_poly_annealing EXCLUDE_FROM_ALL find_conv_optim_poly_annealing.c $<TARGET_OBJECTS:error_sim>)
    target_link_libraries(conv_find_optim_poly_annealing correct_static)
endif()

add_custom_target(tools DEPENDS rs_find_primitive_poly conv_find_libfec_poly conv_find_optim_poly conv_find_optim_poly_annealing)
