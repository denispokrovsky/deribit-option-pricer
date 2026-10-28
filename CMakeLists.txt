cmake_minimum_required(VERSION 3.25)

project(deribit_pricer
    VERSION 0.1.0
    DESCRIPTION "Black-76 pricing, implied volatility and Greeks for Deribit options"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(DP_BUILD_TESTS "Build unit tests (GoogleTest)" ON)
option(DP_BUILD_BENCHMARKS "Build benchmarks (Google Benchmark)" ON)
option(DP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# Warnings apply to our own targets only, never to fetched dependencies.
function(dp_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-sign-conversion
            -Wno-missing-field-initializers)
    endif()
    if(DP_WARNINGS_AS_ERRORS)
        set_target_properties(${target} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
    endif()
endfunction()

add_subdirectory(src)
add_subdirectory(apps)

if(DP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(DP_BUILD_BENCHMARKS)
    add_subdirectory(bench)
endif()
