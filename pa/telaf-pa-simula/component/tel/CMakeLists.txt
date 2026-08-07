cmake_minimum_required(VERSION 3.15)
project(telux_tel_simula)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBRARY_VERSION "1.0.0")
set(LIBRARY_SOVERSION "1")

# nlohmann/json: needed directly by ../sim/*.cpp (payload construction),
# same find-or-fallback pattern as component/common/CMakeLists.txt and
find_package(Threads REQUIRED)

# nlohmann/json: needed directly here for payload construction in the
# Manager .cpp files, even though ModemBridge itself lives in telux_common.
# Same find-or-fallback pattern as component/common/CMakeLists.txt and
# component/data/CMakeLists.txt.
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
set(COMMON_INCLUDE    ${CMAKE_CURRENT_SOURCE_DIR}/../common)
# sml/, so ../sim/*.cpp can `#include "generated/cpp/topics.h"` -- the
# sml/shared/ Code-Gen pipeline output (see gen_cpp.py), same include path
# component/data/CMakeLists.txt adds for its own generated-header use.
set(SML_ROOT_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../sml)
# chart/ header-only HSM/AO framework: ../sim/{CardManager,SubscriptionManager}.*
# (compiled into telux_tel below) `#include <chart/active_object.hpp>` etc.
# telux_common adds chart only via non-propagating include_directories, so it
# is not exported by linking -- add it here explicitly, same as data/CMakeLists.txt.
set(CHART_INCLUDE     ${CMAKE_CURRENT_SOURCE_DIR}/../chart/include)
include_directories(${TELUX_SDK_INCLUDE} ${COMMON_INCLUDE} ${SML_ROOT_INCLUDE} ${CHART_INCLUDE})

# ../sim/*.cpp physically lives in a sibling dir (not its own
# add_subdirectory target) because PhoneFactory::getInstance() must have a
# single definition across the whole telux_tel .so -- this file's stub
# PhoneFactoryStub delegates getCardManager()/getSubscriptionManager() to
# SimulaSimFactory, so the two source sets need to link into one target.
add_library(telux_tel SHARED
    TelephonyManager.cpp
    ../sim/SimCard.cpp
    ../sim/Subscription.cpp
    ../sim/CardManager.cpp
    ../sim/SubscriptionManager.cpp
    ../sim/MultiSimManager.cpp
    ../sim/SimFactory.cpp
    Phone.cpp
    PhoneManager.cpp
    ServingSystemManager.cpp
    NetworkSelectionManager.cpp
    VoiceServiceInfo.cpp   # taf_radio_GetNetRegState
    SignalStrength.cpp     # taf_radio_MeasureSignalMetrics + per-RAT getters
    CellInfo.cpp           # taf_radio_GetServingCellId / GetCurrentNetworkMccMnc
)
set_target_properties(telux_tel PROPERTIES
    VERSION ${LIBRARY_VERSION}
    SOVERSION ${LIBRARY_SOVERSION}
    OUTPUT_NAME telux_tel)

target_link_libraries(telux_tel PUBLIC
    nlohmann_json::nlohmann_json
    Threads::Threads
    telux_common
)

target_compile_options(telux_tel PRIVATE -fvisibility=default)

install(TARGETS telux_tel LIBRARY DESTINATION lib)

