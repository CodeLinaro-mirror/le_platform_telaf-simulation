cmake_minimum_required(VERSION 2.8)

project(IvssClientTest)

message(STATUS "PROJECT_NAME: ${PROJECT_NAME}")

# CommonAPI
include(FindPkgConfig)

if(NOT DEFINED SIMULATION_DEPS_ROOTFS OR "${SIMULATION_DEPS_ROOTFS}" STREQUAL "")
    message(FATAL_ERROR "SIMULATION_DEPS_ROOTFS is not set; cannot locate CommonAPI/vsomeip dependencies")
endif()

set(PKG_CONFIG_SYSROOT_DIR "${SIMULATION_DEPS_ROOTFS}")
set(IVSS_DEPS_CMAKE_DIR "${PKG_CONFIG_SYSROOT_DIR}/lib/cmake")
set(IVSS_DEPS_USR_CMAKE_DIR "${PKG_CONFIG_SYSROOT_DIR}/usr/lib/cmake")

list(INSERT CMAKE_PREFIX_PATH 0
    "${IVSS_DEPS_CMAKE_DIR}"
    "${IVSS_DEPS_USR_CMAKE_DIR}"
    "${PKG_CONFIG_SYSROOT_DIR}/lib"
    "${PKG_CONFIG_SYSROOT_DIR}/usr/lib"
    "${PKG_CONFIG_SYSROOT_DIR}"
)

if(EXISTS "${IVSS_DEPS_CMAKE_DIR}/vsomeip3/vsomeip3Config.cmake")
    set(vsomeip3_DIR "${IVSS_DEPS_CMAKE_DIR}/vsomeip3")
elseif(EXISTS "${IVSS_DEPS_USR_CMAKE_DIR}/vsomeip3/vsomeip3Config.cmake")
    set(vsomeip3_DIR "${IVSS_DEPS_USR_CMAKE_DIR}/vsomeip3")
else()
    message(FATAL_ERROR "Required package vsomeip3 not found. Searched: ${IVSS_DEPS_CMAKE_DIR}/vsomeip3 and ${IVSS_DEPS_USR_CMAKE_DIR}/vsomeip3")
endif()

if(EXISTS "${IVSS_DEPS_CMAKE_DIR}/CommonAPI-3.2.0/CommonAPIConfig.cmake")
    set(CommonAPI_DIR "${IVSS_DEPS_CMAKE_DIR}/CommonAPI-3.2.0")
elseif(EXISTS "${IVSS_DEPS_USR_CMAKE_DIR}/CommonAPI-3.2.0/CommonAPIConfig.cmake")
    set(CommonAPI_DIR "${IVSS_DEPS_USR_CMAKE_DIR}/CommonAPI-3.2.0")
else()
    message(FATAL_ERROR "Required package CommonAPI 3.2.0 not found. Searched: ${IVSS_DEPS_CMAKE_DIR}/CommonAPI-3.2.0 and ${IVSS_DEPS_USR_CMAKE_DIR}/CommonAPI-3.2.0")
endif()

if(EXISTS "${IVSS_DEPS_CMAKE_DIR}/CommonAPI-SomeIP-3.2.0/CommonAPI-SomeIPConfig.cmake")
    set(CommonAPI-SomeIP_DIR "${IVSS_DEPS_CMAKE_DIR}/CommonAPI-SomeIP-3.2.0")
elseif(EXISTS "${IVSS_DEPS_USR_CMAKE_DIR}/CommonAPI-SomeIP-3.2.0/CommonAPI-SomeIPConfig.cmake")
    set(CommonAPI-SomeIP_DIR "${IVSS_DEPS_USR_CMAKE_DIR}/CommonAPI-SomeIP-3.2.0")
else()
    message(FATAL_ERROR "Required package CommonAPI-SomeIP 3.2.0 not found. Searched: ${IVSS_DEPS_CMAKE_DIR}/CommonAPI-SomeIP-3.2.0 and ${IVSS_DEPS_USR_CMAKE_DIR}/CommonAPI-SomeIP-3.2.0")
endif()

message(STATUS "Searching IVSS dependencies in: ${SIMULATION_DEPS_ROOTFS}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message(STATUS "TELAF_SIMULATION_ENABLE_IVSS_DATA: ${TELAF_SIMULATION_ENABLE_IVSS_DATA}")
message(STATUS "TELAF_SIMULATION_ENABLE_IVSS_TELEPHONY: ${TELAF_SIMULATION_ENABLE_IVSS_TELEPHONY}")

if(DEFINED TELAF_SIMULATION_ENABLE_IVSS_DATA AND "${TELAF_SIMULATION_ENABLE_IVSS_DATA}" STREQUAL "y")
    add_definitions(-DTELAF_SIMULATION_ENABLE_IVSS_DATA)
endif()

if(DEFINED TELAF_SIMULATION_ENABLE_IVSS_TELEPHONY AND "${TELAF_SIMULATION_ENABLE_IVSS_TELEPHONY}" STREQUAL "y")
    add_definitions(-DTELAF_SIMULATION_ENABLE_IVSS_TELEPHONY)
endif()

# CommonAPI library
find_package(vsomeip3 3.1.0 REQUIRED CONFIG)
find_package(CommonAPI 3.2.0 REQUIRED CONFIG)
find_package(CommonAPI-SomeIP 3.2.0 REQUIRED CONFIG)

# Source Files
set(PRJ_SRC_PATH src)
set(PRJ_SRC_GEN_PATH src-gen)
set(PRJ_INCLUDE_DIRS src-gen/)

# Paths
include_directories(
    ${PRJ_INCLUDE_DIRS}
    ${COMMONAPI_INCLUDE_DIRS}
    ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
    ${SIMULATION_DEPS_ROOTFS}/include
    ${SIMULATION_DEPS_ROOTFS}/include/CommonAPI-3.2
    ${VSOMEIP_INCLUDE_DIRS}
)

# Application
FILE(GLOB PRJ_SRC_LIB_SRCS ${PRJ_SRC_PATH}/*cpp)
FILE(GLOB_RECURSE PRJ_SRC_GEN_SRCS ${PRJ_SRC_GEN_PATH}/*.cpp)

add_executable(${PROJECT_NAME}
    ${PRJ_SRC_LIB_SRCS}
    ${PRJ_SRC_GEN_SRCS}
)

target_link_libraries(${PROJECT_NAME} CommonAPI CommonAPI-SomeIP vsomeip3)

target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-g;-Wall;-Werror;-fno-omit-frame-pointer;-fmacro-prefix-map=${LEGATO_ROOT}=.;-fmacro-prefix-map=${TELAF_ROOT}=.;-fmacro-prefix-map=${LEGATO_SYSROOT}=.>")

install (
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ./ COMPONENT bin)

set(EXAMPLE_CONFIG_FILES
    "commonapi-test.ini"
    "vsomeip-test.json"
)

install (
    FILES ${EXAMPLE_CONFIG_FILES} DESTINATION conf COMPONENT config
)
