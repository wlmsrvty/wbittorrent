cmake_minimum_required(VERSION 3.13)

project(
  ownbittorrent
  VERSION 0.1
  DESCRIPTION "Own bittorrent implementation"
  LANGUAGES CXX)

add_compile_options(-Wall -Wextra -pedantic)

option(BUILD_TESTS "Build test programs" OFF)

set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

include(FetchContent)

# bittorrent library
add_subdirectory(src)

# cli app
add_subdirectory(cli)

# tests
if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
