cmake_minimum_required(VERSION 3.29)
project(publisher C)

set(CMAKE_C_STANDARD 99)

find_package(open62541 REQUIRED HINTS "/home/drblah/Projects/ABB/OPC-UA/open62541_installed")

include(ExternalProject)
ExternalProject_Add(open62541_src
        SOURCE_DIR "/home/drblah/Projects/ABB/OPC-UA/open62541"
        BINARY_DIR "/home/drblah/Projects/ABB/OPC-UA/open62541/cmake_build"
        INSTALL_DIR "/home/drblah/Projects/ABB/OPC-UA/open62541_installed"
        CONFIGURE_COMMAND ${CMAKE_COMMAND} "/home/drblah/Projects/ABB/OPC-UA/open62541" -DUA_BUILD_TOOLS=ON -DUA_BUILD_EXAMPLES=ON
        BUILD_COMMAND ${CMAKE_COMMAND} --build "/home/drblah/Projects/ABB/OPC-UA/open62541/cmake_build" --config ${CMAKE_BUILD_TYPE}
        EXCLUDE_FROM_ALL TRUE
        STEP_TARGETS build
        INSTALL_COMMAND ${CMAKE_COMMAND} --install "/home/drblah/Projects/ABB/OPC-UA/open62541/cmake_build" --prefix "/home/drblah/Projects/ABB/OPC-UA/open62541_installed" --config ${CMAKE_BUILD_TYPE}
        BUILD_BYPRODUCTS /home/drblah/Projects/ABB/OPC-UA/open62541_installed/${CMAKE_INSTALL_LIBDIR}/${LIB_NAME}
)

add_executable(publisher main.c)
add_dependencies(publisher open62541_src)

target_link_libraries(publisher open62541::open62541)