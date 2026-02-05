project(TAF_PA_COMMON C)

set(LIBRARY_VERSION "1.0.0")
set(LIBRARY_SOVERSION "1")
set(TARGET_NAME taf_pa_common)
set(OUTPUT_NAME Component_${TARGET_NAME})

# common headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)

add_library(${TARGET_NAME} SHARED taf_pa_common.c)

set_target_properties(${TARGET_NAME} PROPERTIES
    VERSION ${LIBRARY_VERSION}
    SOVERSION ${LIBRARY_SOVERSION}
    OUTPUT_NAME ${OUTPUT_NAME}
)

install(TARGETS ${TARGET_NAME}
    LIBRARY DESTINATION lib
)
