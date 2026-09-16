cmake_minimum_required(VERSION 3.28)

# The macOS floor is 26.0 (R8). It has to be set before project() so the
# compiler identification runs against the same target triple as the build.
if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "26.0" CACHE STRING "Minimum macOS version")
endif()

project(syntonic LANGUAGES C OBJC)

if(NOT APPLE)
  message(FATAL_ERROR "Syntonic wraps AppKit and only builds on macOS.")
endif()

option(SYNTONIC_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# Debug by default so `leaks` and the sanitizers have symbols to report with.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_OBJC_STANDARD 23)
set(CMAKE_OBJC_STANDARD_REQUIRED ON)

# Flags for Syntonic's own targets only; never inflicted on a consumer.
add_library(syntonic_flags INTERFACE)
target_compile_options(syntonic_flags INTERFACE
  -Wall -Wextra
  $<$<COMPILE_LANGUAGE:OBJC>:-fobjc-arc>
)
# A toolchain update that introduces a new warning would otherwise break every
# project that consumes Syntonic through add_subdirectory.
if(PROJECT_IS_TOP_LEVEL)
  target_compile_options(syntonic_flags INTERFACE -Werror)
endif()

if(SYNTONIC_SANITIZE)
  target_compile_options(syntonic_flags INTERFACE
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -g)
  target_link_options(syntonic_flags INTERFACE -fsanitize=address,undefined)
endif()

file(GLOB SYNTONIC_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.m")

add_library(syntonic STATIC ${SYNTONIC_SOURCES})
target_include_directories(syntonic PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
# R4 promises that handing a wrapper a handle of the wrong class fails to
# compile. Clang only warns about that by default, so consumers inherit the
# upgrade to an error along with the headers.
target_compile_options(syntonic PUBLIC -Werror=incompatible-pointer-types)
target_link_libraries(syntonic
  PRIVATE syntonic_flags
  PUBLIC "-framework AppKit" "-framework Foundation")

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()
  file(GLOB SYNTONIC_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.c")
  foreach(test_source IN LISTS SYNTONIC_TEST_SOURCES)
    cmake_path(GET test_source STEM test_name)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE syntonic syntonic_flags)
    add_test(NAME ${test_name} COMMAND ${test_name})
    # The leaks run is not instrumented, so it needs a .dSYM beside the binary
    # to turn a backtrace into file and line (KTD12).
    add_custom_command(TARGET ${test_name} POST_BUILD
      COMMAND dsymutil $<TARGET_FILE:${test_name}>)
  endforeach()
endif()
