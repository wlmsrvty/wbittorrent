file(GLOB_RECURSE BITTORRENT_SOURCES "*.cpp")

# Sets bittorrent executable location
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

add_executable(bittorrent ${BITTORRENT_SOURCES})
target_link_libraries(bittorrent PRIVATE ${BITTORRENT_LIB_NAME})
