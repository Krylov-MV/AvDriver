cmake_minimum_required(VERSION 3.5)

project(AvDriver LANGUAGES CXX VERSION 0.2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(open62541 REQUIRED)
find_library(libmodbus modbus REQUIRED)

add_executable(AvDriver
    main.cpp
    modbustcpclient.cpp
    modbustcpclient.h
    industrialprotocolutils.cpp
    industrialprotocolutils.h
    opcuaclient.cpp
    opcuaclient.h
    rtm64var.cpp
    rtm64var.h)

target_link_libraries(AvDriver ${libmodbus} open62541)
