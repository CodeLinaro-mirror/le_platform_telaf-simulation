
cmake_minimum_required(VERSION 3.15)
project(telux_common_simula)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBRARY_VERSION "1.0.0")
set(LIBRARY_SOVERSION "1")

set(TELUX_SDK_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk-include)
set(CHART_INCLUDE     ${CMAKE_CURRENT_SOURCE_DIR}/../chart/include)
include_directories(${TELUX_SDK_INCLUDE} ${CHART_INCLUDE})

add_library(telux_common SHARED Common.cpp Log.cpp)
set_target_properties(telux_common PROPERTIES
    VERSION ${LIBRARY_VERSION}
    SOVERSION ${LIBRARY_SOVERSION}
    OUTPUT_NAME telux_common)

target_compile_options(telux_common PRIVATE -fvisibility=default)

install(TARGETS telux_common LIBRARY DESTINATION lib)
