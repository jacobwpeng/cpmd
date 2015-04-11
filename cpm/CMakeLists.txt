cmake_minimum_required(VERSION 2.6)
set(LIBCPM "cpm")
set(CPMD "cpmd")
set(CPM_CLIENT "cpm_client")
list(APPEND LIBCPM_SRCS "cpm_address.cc"
                        "cpm_message.cc"
                        "cpm_udp_socket.cc"
                        "cpm_options.cc"
                        "cpm_impl.cc"
                        )
add_library(${LIBCPM} STATIC ${LIBCPM_SRCS})
target_link_libraries(${LIBCPM} "alpha")

list(APPEND CPMD_SRCS   "cpm_address.cc"
                        "cpm_message.cc"
                        "cpm_client_info.cc" 
                        "cpm_server.cc"
                        "cpm_main.cc"
                        )
add_executable(${CPMD} ${CPMD_SRCS})
target_link_libraries(${CPMD} "alpha")

list(APPEND CPM_CLIENT_SRCS "cpm_client_main.cc")
add_executable(${CPM_CLIENT} ${CPM_CLIENT_SRCS})
target_link_libraries(${CPM_CLIENT} ${LIBCPM})
