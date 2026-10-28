find_package(Threads REQUIRED)

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

set(TELUX_SDK_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../sdk-include)
set(SIMULA_COMMON_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../../common)
set(CHART_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../../chart/include)
set(SML_ROOT_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../sml)

add_library(taf_prop_radio SHARED
    taf_prop_radio.cpp
)

target_include_directories(taf_prop_radio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${TELUX_SDK_INCLUDE}
    ${SIMULA_COMMON_INCLUDE}
    ${CHART_INCLUDE}
    ${SML_ROOT_INCLUDE}
)

target_compile_options(taf_prop_radio PRIVATE -fvisibility=default)

target_link_libraries(taf_prop_radio PRIVATE
    taf_prop_common
    telux_common
    nlohmann_json::nlohmann_json
    Threads::Threads
)

set_target_properties(taf_prop_radio PROPERTIES
    VERSION     1.0.0
    SOVERSION   1
    OUTPUT_NAME Component_taf_prop_radio)

install(TARGETS taf_prop_radio LIBRARY DESTINATION lib)
