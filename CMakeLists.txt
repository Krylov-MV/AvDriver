cmake_minimum_required(VERSION 3.5)

project(AvGateway LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEADER_DIR ${PROJECT_SOURCE_DIR}/include)
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(LIB_DIR ${PROJECT_SOURCE_DIR}/lib)

set(HEADERS
    ${HEADER_DIR}/industrialprotocolutils.h
    ${HEADER_DIR}/modbustcpclient.h
    ${HEADER_DIR}/opcuaclient.h
)

set(SOURCES
    ${SOURCE_DIR}/main.cpp
    ${SOURCE_DIR}/industrialprotocolutils.cpp
    ${SOURCE_DIR}/modbustcpclient.cpp
    ${SOURCE_DIR}/opcuaclient.cpp
)

find_package(Threads REQUIRED)

include_directories(
    ${HEADER_DIR}
)

add_executable(${PROJECT_NAME}
    ${HEADERS}
    ${SOURCES}
)

find_library(LIBMODBUS_LIB NAMES modbus PATHS ${LIB_DIR})
find_library(OPEN62541_LIB NAMES open62541 PATHS ${LIB_DIR})
find_library(TINYXML2_LIB NAMES tinyxml2 PATHS ${LIB_DIR})

target_link_libraries(${PROJECT_NAME}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LIBMODBUS_LIB}
    ${OPEN62541_LIB}
    ${TINYXML2_LIB}
)
