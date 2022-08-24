project(arp_netlink C)
cmake_minimum_required(VERSION 3.18)
add_compile_options(-std=c11 -Wall)
set(CMAKE_C_STANDARD 11)

include_directories("/usr/include")

add_executable(arp main.c libnlarpcache.c)