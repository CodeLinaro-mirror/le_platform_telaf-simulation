
cmake_minimum_required(VERSION 3.14)
project(chart
  VERSION 0.1.0
  LANGUAGES CXX
  DESCRIPTION "Tiny hierarchical state machine + active object framework"
)

option(CHART_BUILD_TESTS    "Build chart tests"    ON)
option(CHART_BUILD_EXAMPLES "Build chart examples" ON)

# ---- Library: header-only INTERFACE target -----------------------------------
add_library(chart INTERFACE)
add_library(chart::chart ALIAS chart)

target_include_directories(chart INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_features(chart INTERFACE cxx_std_17)
find_package(Threads REQUIRED)
target_link_libraries(chart INTERFACE Threads::Threads)

# ---- Install rules ------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(DIRECTORY include/chart
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS chart EXPORT chartTargets)
install(EXPORT chartTargets
        FILE chartTargets.cmake
        NAMESPACE chart::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/chart)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/chartConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/chartConfig.cmake"
"include(\"\${CMAKE_CURRENT_LIST_DIR}/chartTargets.cmake\")\n")

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/chartConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/chartConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/chart)

# ---- Tests --------------------------------------------------------------------
if(CHART_BUILD_TESTS)
  enable_testing()
  set(CHART_TESTS test_hsm test_active_object test_time_event test_pub_sub test_spy)
  foreach(t IN LISTS CHART_TESTS)
    add_executable(${t} tests/${t}.cpp)
    target_link_libraries(${t} PRIVATE chart::chart)
    target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME ${t} COMMAND ${t})
  endforeach()

  # T-01 regression: exercises bounded-break in Hsm::init()/dispatch() under
  # -DNDEBUG (CHART_ASSERT is a no-op there — only the break protects the
  # stack from writes past StateFn path[kMaxNest]).
  add_executable(test_hsm_deep_nest tests/test_hsm_deep_nest.cpp)
  target_link_libraries(test_hsm_deep_nest PRIVATE chart::chart)
  target_compile_options(test_hsm_deep_nest PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_definitions(test_hsm_deep_nest PRIVATE NDEBUG)
  add_test(NAME test_hsm_deep_nest COMMAND test_hsm_deep_nest)
endif()

# ---- Examples -----------------------------------------------------------------
if(CHART_BUILD_EXAMPLES)
  add_executable(chart_blinky    examples/blinky.cpp)
  add_executable(chart_ping_pong examples/ping_pong.cpp)
  add_executable(chart_dining    examples/dining_philosophers.cpp)
  foreach(t chart_blinky chart_ping_pong chart_dining)
    target_link_libraries(${t} PRIVATE chart::chart)
    target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic)
  endforeach()
endif()
