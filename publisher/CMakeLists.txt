cmake_minimum_required(VERSION 3.29)
project(publisher C)

set(CMAKE_C_STANDARD 99)

#find_package(open62541 REQUIRED HINTS "/home/drblah/Projects/ABB/OPC-UA/open62541_installed")

add_subdirectory("/home/drblah/Projects/ABB/OPC-UA/open62541/" open62541 EXCLUDE_FROM_ALL)

add_executable(publisher main.c)

target_link_libraries(publisher open62541)