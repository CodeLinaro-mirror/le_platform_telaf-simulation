cmake_minimum_required(VERSION 3.15)
project(telaf_np)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Standalone debug build (build_pa.sh) -- when configured on its own rather
# than via the parent's add_subdirectory, the sibling telux_common target does
# not exist, so stand in an imported one from the parent's staging/lib. Also
# reproduce the parent's tree-wide -fvisibility=hidden, so the per-target
# -fvisibility=default overrides are exercised the same way here.
if(TELAF_NP_STANDALONE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN:${TELAF_NP_SIMULA_LIB_DIR}")

    find_library(TELUX_COMMON_LIB
        NAMES telux_common
        PATHS ${TELAF_NP_SIMULA_LIB_DIR}
        NO_DEFAULT_PATH REQUIRED)
    add_library(telux_common SHARED IMPORTED)
    set_target_properties(telux_common PROPERTIES
        IMPORTED_LOCATION ${TELUX_COMMON_LIB})

    # ModemBridge.hpp includes <mosquitto.h>. In the parent build the real
    # telux_common target carries this include dir; an imported target has to
    # be told. Note this makes the standalone build a *container-side* tool:
    # libmosquitto-dev is installed in the develop docker image, not
    # necessarily on the bare host.
    find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h)
    if(NOT MOSQUITTO_INCLUDE_DIR)
        message(FATAL_ERROR
            "mosquitto.h not found. component/telaf-np/build_pa.sh is meant to "
            "run inside the develop docker image (which installs "
            "libmosquitto-dev); on a bare host, build via the parent instead: "
            "bash pa/telaf-pa-simula/build_pa.simula.sh")
    endif()
    set_target_properties(telux_common PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${MOSQUITTO_INCLUDE_DIR})
endif()

# Only the domains that actually build today. taf_prop_common must come first
# -- every other domain links it.
set(TELAF_NP_MODULE_LIST
    component/taf_prop_common
    component/taf_prop_pms
)

foreach(mod ${TELAF_NP_MODULE_LIST})
    add_subdirectory(${mod})
endforeach()
