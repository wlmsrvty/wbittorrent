file(GLOB HEADER_LIST CONFIGURE_DEPENDS "lib/**/*.hpp")

add_library(${BITTORRENT_LIB_NAME}
    bencode.cpp
    bencode.hpp
    ${HEADER_LIST})

target_include_directories(${BITTORRENT_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})