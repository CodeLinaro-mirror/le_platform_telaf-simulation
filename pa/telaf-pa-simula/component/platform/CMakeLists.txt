cmake_minimum_required(VERSION 3.15)
project(telux_device_info)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBRARY_VERSION "1.0.0")
set(LIBRARY_SOVERSION "1")

# SDK headers live beside pa/telaf-pa-simula/sdk-include/
set(TELUX_SDK_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk-include)
include_directories(${TELUX_SDK_INCLUDE})

# Find Boost (header-only, no components needed)
find_package(Boost QUIET)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found. Please install Boost (e.g. apt install libboost-dev). Cannot continue without Boost.")
endif()


add_library(telux_platform SHARED DeviceInfoManager.cpp)
target_compile_options(telux_platform PRIVATE -fvisibility=default)

# Link telux_common so that telux::common::simula::log_impl
target_link_libraries(telux_platform PRIVATE telux_common)

# Expose the common Log.hpp header path to this target
target_include_directories(telux_platform PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${Boost_INCLUDE_DIRS}
)

# Set library properties
set_target_properties(telux_platform PROPERTIES
    VERSION ${LIBRARY_VERSION}
    SOVERSION ${LIBRARY_SOVERSION}
    OUTPUT_NAME telux_platform
)

# Install
install(TARGETS telux_platform LIBRARY DESTINATION lib)


