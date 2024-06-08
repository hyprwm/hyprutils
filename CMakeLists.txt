cmake_minimum_required(VERSION 3.19)

set(HYPRUTILS_VERSION "0.1.0")
add_compile_definitions(HYPRUTILS_VERSION="${HYPRUTILS_VERSION}")

project(hyprutils
    VERSION ${HYPRUTILS_VERSION}
    DESCRIPTION "A library and toolkit for the Hyprland cursor format"
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

add_library(hyprutils SHARED ${SRCFILES})
target_include_directories( hyprutils
    PUBLIC "./include"
    PRIVATE "./src"
)
set_target_properties(hyprutils PROPERTIES
    VERSION ${hyprutils_VERSION}
    SOVERSION 0
)

# tests
add_custom_target(tests)

add_executable(hyprutils_memory "tests/memory.cpp")
target_link_libraries(hyprutils_memory PRIVATE hyprutils)
add_test(NAME "Memory" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests COMMAND hyprutils_memory "memory")
add_dependencies(tests hyprutils_memory)

add_executable(hyprutils_string "tests/string.cpp")
target_link_libraries(hyprutils_string PRIVATE hyprutils)
add_test(NAME "String" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests COMMAND hyprutils_string "string")
add_dependencies(tests hyprutils_string)

add_executable(hyprutils_signal "tests/signal.cpp")
target_link_libraries(hyprutils_signal PRIVATE hyprutils)
add_test(NAME "Signal" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests COMMAND hyprutils_signal "signal")
add_dependencies(tests hyprutils_signal)

# Installation
install(TARGETS hyprutils)
install(DIRECTORY "include/hyprutils" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_BINARY_DIR}/hyprutils.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
