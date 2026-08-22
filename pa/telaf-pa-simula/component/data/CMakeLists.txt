# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#
# Simulated telux::data SDK implementation.
#
# Reimplemented: DataFactory + DataConnectionManager (SimulaDataCall +
# SimulaDataCallSession + SimulaDataConnectionManager bundled) +
# DataProfileManager + ServingSystemManager. ModemBridge/Envelope/
# ListenerDispatchAO moved to component/common/ (shared across domains,
# linked via telux_common) -- this target no longer owns MQTT transport or
# JSON directly.

cmake_minimum_required(VERSION 3.15)
project(telux_data_simula)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBRARY_VERSION "1.0.0")
set(LIBRARY_SOVERSION "1")

find_package(Threads REQUIRED)

# nlohmann/json: needed directly (payload construction in the Manager/
# Session .cpp files), even though ModemBridge itself now lives in
# telux_common. Same find-or-fallback pattern as component/common/CMakeLists.txt.
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

# Include paths.
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

# ---------------------------------------------------------------------------
# libtelux_data.so — simulated telux::data SDK.

add_library(telux_data SHARED
    DataProfile.cpp
    DataConnectionManager.cpp
    DataProfileManager.cpp
    ServingSystemManager.cpp
    DataFactory.cpp
)

target_link_libraries(telux_data PUBLIC
    nlohmann_json::nlohmann_json
    Threads::Threads
    telux_common
)

# Parent CMakeLists.txt sets `-fvisibility=hidden` for the whole simula PA
# build. For telux_data we want symbols visible: the library exports the
# telux::data ABI consumed by telaf-pa-target. A trailing -fvisibility=default
# flag wins over the earlier one.
target_compile_options(telux_data PRIVATE -fvisibility=default)

set_target_properties(telux_data PROPERTIES
    VERSION       ${LIBRARY_VERSION}
    SOVERSION     ${LIBRARY_SOVERSION}
    OUTPUT_NAME   telux_data
)

install(TARGETS telux_data LIBRARY DESTINATION lib)
