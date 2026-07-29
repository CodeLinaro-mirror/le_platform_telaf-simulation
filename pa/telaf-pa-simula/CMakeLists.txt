cmake_minimum_required(VERSION 3.10)
project(telaf_pa_all)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add default RPATH
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN:")

set(MODULE_LIST
    component/common
    component/tel
    component/platform
    component/data
)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(SML_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../sml)

# ---------------------------------------------------------------------------
# Contract code generation.
#
# `sml/generated/` is gitignored (regenerated here, never hand-edited). The
# telux_data .cpp files `#include "generated/cpp/topics.h"`, so the C++ build
# depends on these headers -- a fresh checkout has no generated/ and would
# otherwise fail to compile. Three generators run:
#   - shared/tools/gen_cpp.py     -> generated/cpp/{topics,payloads,validators}.h
#   - shared/tools/gen_python.py  -> generated/python/{topics,payloads,validators}.py
#   - control/tools/gen_python.py -> generated/python/{test_*,action_registry}.py
# Each is invoked by absolute path, so its own dir lands on sys.path[0] and the
# bare `import schema_common` resolves without a WORKING_DIRECTORY.
set(GEN_CPP_HEADERS
    ${SML_ROOT}/generated/cpp/topics.h
    ${SML_ROOT}/generated/cpp/payloads.h
    ${SML_ROOT}/generated/cpp/validators.h
)
file(GLOB_RECURSE CODEGEN_INPUTS CONFIGURE_DEPENDS
    ${SML_ROOT}/shared/registry/*.yaml
    ${SML_ROOT}/shared/schemas/*.json
    ${SML_ROOT}/shared/tools/*.py
    ${SML_ROOT}/shared/tools/templates/*.j2
    ${SML_ROOT}/control/registry/*.yaml
    ${SML_ROOT}/control/schemas/*.json
    ${SML_ROOT}/control/tools/*.py
    ${SML_ROOT}/control/tools/templates/*.j2
)
add_custom_command(
    OUTPUT ${GEN_CPP_HEADERS}
    COMMAND ${Python3_EXECUTABLE} ${SML_ROOT}/shared/tools/gen_cpp.py
    COMMAND ${Python3_EXECUTABLE} ${SML_ROOT}/shared/tools/gen_python.py
    COMMAND ${Python3_EXECUTABLE} ${SML_ROOT}/control/tools/gen_python.py
    DEPENDS ${CODEGEN_INPUTS}
    COMMENT "Generating contract code (shared C++/Python + control Python -> sml/generated/)"
    VERBATIM
)
add_custom_target(simula_codegen ALL DEPENDS ${GEN_CPP_HEADERS})

# Blocks the build if config/scenarios/*.yaml has a cross-reference error --
# catches a stale/renamed device/environment id before it ships in a config
# change nobody ran the validator on manually. Runs after codegen so the
# generated python it may import is present.
add_custom_target(validate_scenario ALL
    COMMAND ${Python3_EXECUTABLE} ${SML_ROOT}/config/tools/validate_scenario.py
    WORKING_DIRECTORY ${SML_ROOT}
    COMMENT "Running validate_scenario.py (config/scenarios/*.yaml cross-reference checks)"
    VERBATIM
)
add_dependencies(validate_scenario simula_codegen)

foreach(module ${MODULE_LIST})
    message(STATUS "Including module: ${module}")
    add_subdirectory(${module})
endforeach()

# Targets that #include sml/generated/cpp/*.h -- keep in sync with a grep for
# `generated/cpp/` under component/. A missing entry causes a parallel-build
# race on clean checkouts; a stale entry fails here loudly instead of silently
# dropping the dependency (add_dependencies on a non-target is a no-op).
set(CODEGEN_CONSUMERS
    telux_common   # component/common/ModemBridge.cpp -> validators.h
    telux_data     # component/data/*.cpp             -> topics.h
)
foreach(consumer ${CODEGEN_CONSUMERS})
    if(NOT TARGET ${consumer})
        message(FATAL_ERROR
            "CODEGEN_CONSUMERS lists '${consumer}', which is not a target. "
            "It was probably renamed or removed -- update the list.")
    endif()
    add_dependencies(${consumer} simula_codegen)
endforeach()
