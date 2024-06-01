cmake_minimum_required(VERSION 3.13)

project(
  ownbittorrent
  VERSION 0.1
  DESCRIPTION "Own bittorrent implementation"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

# set the bittorrent library name
set(BITTORRENT_LIB_NAME bittorrent_library)

include(FetchContent)

# bittorrent library
add_subdirectory(src)

# cli app
add_subdirectory(cli)

# tests
enable_testing()
add_subdirectory(tests)