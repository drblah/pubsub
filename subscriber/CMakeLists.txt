cmake_minimum_required(VERSION 3.29)
project(subscriber C)

set(CMAKE_C_STANDARD 99)

find_package(open62541 REQUIRED HINTS "/home/drblah/Projects/ABB/OPC-UA/open62541_installed")

add_executable(subscriber main.c)

target_link_libraries(subscriber open62541::open62541)