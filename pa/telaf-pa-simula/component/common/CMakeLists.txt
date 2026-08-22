
cmake_minimum_required(VERSION 3.15)
project(telux_common_simula)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBRARY_VERSION "1.0.0")
set(LIBRARY_SOVERSION "1")

set(TELUX_SDK_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk-include)
set(CHART_INCLUDE     ${CMAKE_CURRENT_SOURCE_DIR}/../chart/include)
# sml/, so ModemBridge.cpp can `#include "generated/cpp/validators.h"` -- the
# sml/simula_shared/ Code-Gen pipeline output (see gen_cpp.py), same pattern
# component/data/CMakeLists.txt already uses for topics.h.
set(SML_ROOT_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../sml)
include_directories(${TELUX_SDK_INCLUDE} ${CHART_INCLUDE} ${SML_ROOT_INCLUDE})

find_package(Threads REQUIRED)

# nlohmann/json: prefer CMake config package (shipped by nlohmann-json3-dev),
# fall back to find_path so older or non-cmake-aware installs still work.
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

# libmosquitto: the MQTT client library ModemBridge uses directly. Its
# port==0 mechanism connects over a UDS -- verified with mosquitto_pub/sub
# --unix against this tree's broker. ModemBridge is the only consumer; every
# other simula-pa domain reaches MQTT only through IModemBridge.
find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h)
find_library(MOSQUITTO_LIBRARY NAMES mosquitto)
if(NOT MOSQUITTO_INCLUDE_DIR OR NOT MOSQUITTO_LIBRARY)
    message(FATAL_ERROR
        "libmosquitto not found. Install `libmosquitto-dev` on the build host "
        "(the develop docker image installs this automatically).")
endif()
message(STATUS "libmosquitto: include=${MOSQUITTO_INCLUDE_DIR} lib=${MOSQUITTO_LIBRARY}")
include_directories(${MOSQUITTO_INCLUDE_DIR})

# valijson: header-only JSON-Schema validator used by ModemBridge to
# log-only-validate MPSS rsp/ind payloads against sml/generated/cpp/
# validators.h's embedded schemas.
# No apt package exists on this build host's mirror for valijson or
# nlohmann-json-schema-validator (checked jammy/universe -- absent), so
# vendor via FetchContent instead of find_package, pinned to a tagged
# release. Needs network access at CMake-configure time.
include(FetchContent)
FetchContent_Declare(
    valijson
    GIT_REPOSITORY https://github.com/tristanpenman/valijson.git
    GIT_TAG v1.1.3
    GIT_SUBMODULES ""  # header-only core doesn't need valijson's own
                        # tests/examples thirdparty submodules (googletest,
                        # rapidjson, nlohmann-json, etc); skip to avoid an
                        # unbounded recursive clone at configure time.
)
set(valijson_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(valijson_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(valijson)

# UDS vs TCP transport DEFAULT:
#   ON (default) = unix domain socket at /tmp/simula-mqtt.sock,
#   OFF          = tcp://localhost:1883.
# This is only the *default*; BrokerConfig::use_uds lets a caller pick either
# transport at runtime. libmosquitto reaches a UDS by passing port==0 with the
# socket path as host (the same mechanism `mosquitto_pub --unix` uses); TCP is
# host + non-zero port. Both work from one binary, no special build flags.
option(SIMULA_MQTT_UDS "Default MQTT transport to UDS (vs TCP loopback)" ON)

add_library(telux_common SHARED
    Common.cpp
    Log.cpp
    Envelope.cpp
    ListenerDispatchAO.cpp
    ModemBridge.cpp
)
set_target_properties(telux_common PROPERTIES
    VERSION ${LIBRARY_VERSION}
    SOVERSION ${LIBRARY_SOVERSION}
    OUTPUT_NAME telux_common)

target_link_libraries(telux_common PUBLIC
    nlohmann_json::nlohmann_json
    ValiJSON::valijson
    ${MOSQUITTO_LIBRARY}
    Threads::Threads
)

if(SIMULA_MQTT_UDS)
    target_compile_definitions(telux_common PRIVATE
        SIMULA_MQTT_DEFAULT_UDS=1
        SIMULA_MQTT_UDS_SOCKET="/tmp/simula-mqtt.sock")
else()
    target_compile_definitions(telux_common PRIVATE
        SIMULA_MQTT_DEFAULT_UDS=0
        SIMULA_MQTT_UDS_SOCKET="/tmp/simula-mqtt.sock")
endif()

target_compile_options(telux_common PRIVATE -fvisibility=default)

install(TARGETS telux_common LIBRARY DESTINATION lib)

