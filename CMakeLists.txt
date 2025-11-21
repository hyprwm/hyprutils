cmake_minimum_required(VERSION 3.19)

file(READ "${CMAKE_SOURCE_DIR}/VERSION" VER_RAW)
string(STRIP ${VER_RAW} HYPRUTILS_VERSION)
add_compile_definitions(HYPRUTILS_VERSION="${HYPRUTILS_VERSION}")

project(
  hyprutils
  VERSION ${HYPRUTILS_VERSION}
  DESCRIPTION "Small C++ library for utilities used across the Hypr* ecosystem")

include(CTest)
include(GNUInstallDirs)

set(PREFIX ${CMAKE_INSTALL_PREFIX})
set(INCLUDE ${CMAKE_INSTALL_FULL_INCLUDEDIR})
set(LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR})

configure_file(hyprutils.pc.in hyprutils.pc @ONLY)

set(CMAKE_CXX_STANDARD 23)
add_compile_options(
  -Wall
  -Wextra
  -Wpedantic
  -Wno-unused-parameter
  -Wno-unused-value
  -Wno-missing-field-initializers
  -Wno-narrowing
  -Wno-pointer-arith)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES DEBUG)
  message(STATUS "Configuring hyprutils in Debug")
  add_compile_definitions(HYPRLAND_DEBUG)
else()
  add_compile_options(-O3)
  message(STATUS "Configuring hyprutils in Release")
endif()

file(GLOB_RECURSE SRCFILES CONFIGURE_DEPENDS "src/*.cpp" "include/*.hpp")
file(GLOB_RECURSE PUBLIC_HEADERS CONFIGURE_DEPENDS "include/*.hpp")

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET pixman-1)

add_library(hyprutils SHARED ${SRCFILES})
target_include_directories(
  hyprutils
  PUBLIC "./include"
  PRIVATE "./src")
set_target_properties(hyprutils PROPERTIES VERSION ${hyprutils_VERSION}
                                           SOVERSION 9)
target_link_libraries(hyprutils PkgConfig::deps)

if(BUILD_TESTING)
  # GTest
  find_package(GTest CONFIG REQUIRED)
  include(GoogleTest)
  file(GLOB_RECURSE TESTFILES CONFIGURE_DEPENDS "tests/*.cpp")
  add_executable(hyprutils_tests ${TESTFILES})
  target_compile_options(hyprutils_tests PRIVATE --coverage)
  target_link_options(hyprutils_tests PRIVATE --coverage)
  target_include_directories(
    hyprutils_tests
    PUBLIC "./include"
    PRIVATE "./src" "./src/include" "./protocols" "${CMAKE_BINARY_DIR}")
  target_link_libraries(hyprutils_tests PRIVATE hyprutils GTest::gtest_main
                                                       PkgConfig::deps)
  gtest_discover_tests(hyprutils_tests)
endif()

# Installation
install(TARGETS hyprutils)
install(DIRECTORY "include/hyprutils" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_BINARY_DIR}/hyprutils.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
