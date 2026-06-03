cmake_minimum_required(VERSION 3.10)
project(telaf_pa_target)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Locate the real telaf-pa upstream repo. Under simula-x, ${TELAF_ROOT} is set
# in the container and ${TELAF_ROOT}/../telaf-pa points to the upstream repo.
# Fall back to a relative path for out-of-container manual builds.
if(DEFINED ENV{TELAF_ROOT})
    set(TELAF_PA_TARGET_ROOT $ENV{TELAF_ROOT}/../telaf-pa)
else()
    set(TELAF_PA_TARGET_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../telaf-pa)
endif()
get_filename_component(TELAF_PA_TARGET_ROOT ${TELAF_PA_TARGET_ROOT} ABSOLUTE)
message(STATUS "TELAF_PA_TARGET_ROOT = ${TELAF_PA_TARGET_ROOT}")

# PA public + internal headers
include_directories(${TELAF_PA_TARGET_ROOT}/include)
include_directories(${TELAF_PA_TARGET_ROOT}/component/include)

# telux SDK headers live in pa/telaf-pa-simula/sdk-include/ (built first).
# telux stub libs come from pa/telaf-pa-simula/staging/lib/ (already installed
# before this CMakeLists runs, because simula is compiled before target).
get_filename_component(SIMULA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../telaf-pa-simula ABSOLUTE)
include_directories(${SIMULA_DIR}/sdk-include)
link_directories(${SIMULA_DIR}/staging/lib)

# dlt headers/libs come from the simulation deps rootfs.
include_directories($ENV{SIMULATION_DEPS_ROOTFS}/include)
link_directories($ENV{SIMULATION_DEPS_ROOTFS}/lib)

# Add default RPATH
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN:")

# Only build the components that the simulation currently needs.
set(MODULE_LIST
    component/taf_pa_common
)

foreach(module ${MODULE_LIST})
    message(STATUS "Including module from ${TELAF_PA_TARGET_ROOT}: ${module}")
    add_subdirectory(${TELAF_PA_TARGET_ROOT}/${module}
                     ${CMAKE_CURRENT_BINARY_DIR}/${module})
endforeach()
