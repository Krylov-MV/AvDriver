cmake_minimum_required(VERSION 3.5)

project(AvDriver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

find_library(libmodbus NAMES modbus)

add_executable(AvDriver
    main.cpp
    modbustcpclient.cpp
    modbustcpclient.h
    industrialprotocolutils.cpp
    industrialprotocolutils.h
    opcuaclient.cpp
    opcuaclient.h
    tinyxml2.cpp
    tinyxml2.h)

target_link_libraries(AvDriver ${libmodbus} open62541)

target_link_libraries(AvDriver
    ${CMAKE_THREAD_LIBS_INIT}
)
