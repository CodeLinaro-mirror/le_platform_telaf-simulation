cmake_minimum_required(VERSION 3.15)
project(telaf_pa_power)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# libtelux_power.so — simulated telux::power SDK. Mirrors
# component/data/CMakeLists.txt's telux_data target.

find_package(Threads REQUIRED)

find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
    if(NOT NLOHMANN_JSON_INCLUDE_DIR)
        message(FATAL_ERROR
            "nlohmann/json.hpp not found. Install `nlohmann-json3-dev` on the "
            "build host (the develop docker image installs this automatically).")
    endif()
    message(STATUS "nlohmann/json via find_path: ${NLOHMANN_JSON_INCLUDE_DIR}")
    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
    set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${NLOHMANN_JSON_INCLUDE_DIR}")
endif()

set(TELUX_SDK_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk-include)
set(CHART_INCLUDE     ${CMAKE_CURRENT_SOURCE_DIR}/../chart/include)
set(COMMON_INCLUDE    ${CMAKE_CURRENT_SOURCE_DIR}/../common)
# sml/, so call sites can `#include "generated/cpp/topics.h"` -- the
# sml/simula_shared/ Code-Gen pipeline output (see gen_cpp.py).
set(SML_ROOT_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../sml)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TELUX_SDK_INCLUDE}
    ${CHART_INCLUDE}
    ${COMMON_INCLUDE}
    ${SML_ROOT_INCLUDE}
)

add_library(telux_power SHARED
    PowerFactory.cpp
    TcuActivityManager.cpp
    WakeupManager.cpp
)

target_link_libraries(telux_power PUBLIC
    nlohmann_json::nlohmann_json
    Threads::Threads
    telux_common
)

# Parent CMakeLists.txt sets `-fvisibility=hidden` for the whole simula PA
# build. For telux_power we want symbols visible: the library exports the
# telux::power ABI consumed by telaf-pa-target. A trailing -fvisibility=default
# flag wins over the earlier one.
target_compile_options(telux_power PRIVATE -fvisibility=default)

set_target_properties(telux_power PROPERTIES
    VERSION       1.0.0
    SOVERSION     1
    OUTPUT_NAME   telux_power
)

install(TARGETS telux_power LIBRARY DESTINATION lib)

