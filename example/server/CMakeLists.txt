SET(CPMD "cpmd")
add_executable(${CPMD} 
    port_mapper_command_server.cc 
    port_mapper_message_dispatcher.cc
    port_mapper_main.cc)
target_link_libraries(${CPMD} ${LIB_CPMD_CLIENT} "alpha")
