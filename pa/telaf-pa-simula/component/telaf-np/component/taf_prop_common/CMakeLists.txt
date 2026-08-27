
add_library(taf_prop_common SHARED
    taf_prop_common.c
)

target_include_directories(taf_prop_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# The parent simula build sets -fvisibility=hidden tree-wide; this library
# exports a C ABI, so a trailing -fvisibility=default is required.
target_compile_options(taf_prop_common PRIVATE -fvisibility=default)

set_target_properties(taf_prop_common PROPERTIES
    VERSION     1.0.0
    SOVERSION   1
    OUTPUT_NAME Component_taf_prop_common)

install(TARGETS taf_prop_common LIBRARY DESTINATION lib)
