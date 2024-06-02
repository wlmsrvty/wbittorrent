file (GLOB_RECURSE SOURCES
    "*.cpp"
    "*.hpp"
)

add_library(${BITTORRENT_LIB_NAME}
    ${SOURCES})

target_include_directories(${BITTORRENT_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})