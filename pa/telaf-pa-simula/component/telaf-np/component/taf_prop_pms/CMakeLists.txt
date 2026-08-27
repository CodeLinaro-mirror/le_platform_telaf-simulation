find_package(Threads REQUIRED)

# nlohmann/json: the imported target is created in sibling scopes, not
# inherited here, so resolve it locally (same shape as component/power/).
find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND AND NOT TARGET nlohmann_json::nlohmann_json)
    find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
    if(NOT NLOHMANN_JSON_INCLUDE_DIR)
        message(FATAL_ERROR
            "nlohmann/json.hpp not found. Install `nlohmann-json3-dev` on the "
            "build host (the develop docker image installs this automatically).")
    endif()
    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
    set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${NLOHMANN_JSON_INCLUDE_DIR}")
endif()

# Envelope.hpp -> telux/common/CommonDefines.hpp
set(TELUX_SDK_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../sdk-include)
# IModemBridge.hpp / ModemBridge.hpp / Envelope.hpp
set(SIMULA_COMMON_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../../common)
# ModemBridge.hpp -> chart/active_object.hpp (it is an AO internally)
set(CHART_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../../chart/include)
# so call sites can #include "generated/cpp/topics.h"
set(SML_ROOT_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../sml)

add_library(taf_prop_pa_pms SHARED
    taf_prop_pa_pms.cpp
)

target_include_directories(taf_prop_pa_pms PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${TELUX_SDK_INCLUDE}
    ${SIMULA_COMMON_INCLUDE}
    ${CHART_INCLUDE}
    ${SML_ROOT_INCLUDE}
)

# The parent simula build sets -fvisibility=hidden tree-wide; this library
# exports a C ABI, so a trailing -fvisibility=default is required.
target_compile_options(taf_prop_pa_pms PRIVATE -fvisibility=default)

target_link_libraries(taf_prop_pa_pms PRIVATE
    taf_prop_common
    telux_common              # ModemBridge::instance(), Envelope
    nlohmann_json::nlohmann_json
    Threads::Threads
)

set_target_properties(taf_prop_pa_pms PROPERTIES
    OUTPUT_NAME Component_taf_prop_pa_pms)

install(TARGETS taf_prop_pa_pms LIBRARY DESTINATION lib)
