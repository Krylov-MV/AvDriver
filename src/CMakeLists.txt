cmake_minimum_required(VERSION 3.5)

project(AvDriver LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(open62541 REQUIRED COMPONENTS Events FullNamespace)

add_executable(AvDriver main.cpp modbustcpclient.cpp modbustcpclient.h industrialprotocolutils.cpp industrialprotocolutils.h opcuaclient.cpp opcuaclient.h rtm64var.cpp rtm64var.h)

target_link_libraries(AvDriver libmodbus.so open62541::open62541)

target_link_libraries(AvDriver
    ${CMAKE_THREAD_LIBS_INIT}
)
