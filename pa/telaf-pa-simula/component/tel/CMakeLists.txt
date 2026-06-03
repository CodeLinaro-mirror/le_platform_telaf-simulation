cmake_minimum_required(VERSION 3.15)
project(telux_tel_simula)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBRARY_VERSION "1.0.0")
set(LIBRARY_SOVERSION "1")

set(TELUX_SDK_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk-include)
set(COMMON_INCLUDE    ${CMAKE_CURRENT_SOURCE_DIR}/../common)
include_directories(${TELUX_SDK_INCLUDE} ${COMMON_INCLUDE})

add_library(telux_tel SHARED TelephonyManager.cpp)
set_target_properties(telux_tel PROPERTIES
    VERSION ${LIBRARY_VERSION}
    SOVERSION ${LIBRARY_SOVERSION}
    OUTPUT_NAME telux_tel)

target_compile_options(telux_tel PRIVATE -fvisibility=default)

install(TARGETS telux_tel LIBRARY DESTINATION lib)
