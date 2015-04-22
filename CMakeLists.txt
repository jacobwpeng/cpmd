cmake_minimum_required(VERSION 2.6)

project(cpmd)

set(LIB_CPMD_CLIENT "cpmd_client")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-unused-variable")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-private-field -Wno-unused-const-variable")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")

include_directories(include)
include_directories(alpha)
#list(APPEND srcs src/cpmd_impl.cc)

add_subdirectory(alpha)
add_subdirectory(cpm)
#add_library(${LIB_CPMD_CLIENT} STATIC ${srcs})
#add_subdirectory(example)
