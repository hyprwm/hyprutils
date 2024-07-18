cmake_minimum_required(VERSION 3.19)

file(READ "${CMAKE_SOURCE_DIR}/VERSION" VER_RAW)
string(STRIP ${VER_RAW} HYPRUTILS_VERSION)
add_compile_definitions(HYPRUTILS_VERSION="${HYPRUTILS_VERSION}")

project(hyprutils
    VERSION ${HYPRUTILS_VERSION}
    DESCRIPTION "Small C++ library for utilities used across the Hypr* ecosystem"
)

include(CTest)
include(GNUInstallDirs)

set(PREFIX ${CMAKE_INSTALL_PREFIX})
set(INCLUDE ${CMAKE_INSTALL_FULL_INCLUDEDIR})
set(LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR})

configure_file(hyprutils.pc.in hyprutils.pc @ONLY)

set(CMAKE_CXX_STANDARD 23)

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
target_include_directories( hyprutils
    PUBLIC "./include"
    PRIVATE "./src"
)
set_target_properties(hyprutils PROPERTIES
    VERSION ${hyprutils_VERSION}
    SOVERSION 1
)
target_link_libraries(hyprutils PkgConfig::deps)

# tests
add_custom_target(tests)

add_executable(hyprutils_memory "tests/memory.cpp")
target_link_libraries(hyprutils_memory PRIVATE hyprutils PkgConfig::deps)
add_test(NAME "Memory" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests COMMAND hyprutils_memory "memory")
add_dependencies(tests hyprutils_memory)

add_executable(hyprutils_string "tests/string.cpp")
target_link_libraries(hyprutils_string PRIVATE hyprutils PkgConfig::deps)
add_test(NAME "String" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests COMMAND hyprutils_string "string")
add_dependencies(tests hyprutils_string)

add_executable(hyprutils_signal "tests/signal.cpp")
target_link_libraries(hyprutils_signal PRIVATE hyprutils PkgConfig::deps)
add_test(NAME "Signal" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests COMMAND hyprutils_signal "signal")
add_dependencies(tests hyprutils_signal)

add_executable(hyprutils_math "tests/math.cpp")
target_link_libraries(hyprutils_math PRIVATE hyprutils PkgConfig::deps)
add_test(NAME "Math" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests COMMAND hyprutils_math "math")
add_dependencies(tests hyprutils_math)

# Installation
install(TARGETS hyprutils)
install(DIRECTORY "include/hyprutils" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_BINARY_DIR}/hyprutils.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
