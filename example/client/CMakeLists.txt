set(CPMD_CLIENT_EXAMPLE "cpmd_client_example")
add_executable(${CPMD_CLIENT_EXAMPLE} main.cc)
target_link_libraries(${CPMD_CLIENT_EXAMPLE} ${LIB_CPMD_CLIENT} "alpha")
