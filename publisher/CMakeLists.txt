cmake_minimum_required(VERSION 3.29)
project(publisher C)

set(CMAKE_C_STANDARD 99)

#find_package(open62541 REQUIRED HINTS "/home/drblah/Projects/ABB/OPC-UA/open62541_installed")

# Find the GLib package
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)

add_subdirectory("/home/drblah/Projects/ABB/OPC-UA/open62541/" open62541 EXCLUDE_FROM_ALL)

add_executable(publisher main.c)

target_include_directories(publisher PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(publisher open62541 ${GLIB_LIBRARIES})