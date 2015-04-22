cmake_minimum_required(VERSION 2.6)
set(LIBCPM "cpm")
set(LIBCPM_SERVER "cpm_server")
set(CPMD "cpmd")
set(CPM_CLIENT "cpm_client")
set(CPM_RESOLVE_SERVER "cpm_resolve_server")

find_library(GFLAGS "gflags")

#libcpm
list(APPEND LIBCPM_SRCS "cpm_address.cc"
                        "cpm_message.cc"
                        "cpm_protocol.cc"
                        "cpm_udp_socket.cc"
                        "cpm_options.cc"
                        "cpm_impl.cc"
                        )
add_library(${LIBCPM} STATIC ${LIBCPM_SRCS})
target_link_libraries(${LIBCPM} "alpha")

#libcpm_server
list(APPEND LIBCPM_SERVER_SRCS "cpm_protocol_message_codec.cc"
                                "cpm_address.cc"
                                "cpm_protocol.cc"
                                )
add_library(${LIBCPM_SERVER} STATIC ${LIBCPM_SERVER_SRCS})
target_link_libraries(${LIBCPM_SERVER} "alpha")

#cpmd
list(APPEND CPMD_SRCS "cpm_message.cc"
                        "cpm_message_codec.cc"
                        "cpm_node.cc"
                        "cpm_message_cache_queue.cc"
                        "cpm_client_info.cc" 
                        "cpm_server.cc"
                        "cpm_main.cc"
                        )
add_executable(${CPMD} ${CPMD_SRCS})
target_link_libraries(${CPMD}  ${GFLAGS} ${LIBCPM_SERVER})

#cpm client exmaple
list(APPEND CPM_CLIENT_SRCS "cpm_client_main.cc")
add_executable(${CPM_CLIENT} ${CPM_CLIENT_SRCS})
target_link_libraries(${CPM_CLIENT} ${LIBCPM})

# resolve server
list(APPEND CPM_RESOLVE_SERVER_SRCS "cpm_resolve_server_conf.cc"
                                      "cpm_resolve_server.cc"
                                      "cpm_resolve_server_main.cc"
                                        )
add_executable(${CPM_RESOLVE_SERVER} ${CPM_RESOLVE_SERVER_SRCS})
target_link_libraries(${CPM_RESOLVE_SERVER} ${LIBCPM_SERVER})
