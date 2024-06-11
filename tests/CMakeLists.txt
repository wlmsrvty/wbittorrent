# Testing library
# Add Catch2::Catch2
Include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.4.0 # or a later release
)
FetchContent_MakeAvailable(Catch2)

add_executable(bencode_test bencode_decode_test.cpp)
target_compile_features(bencode_test PRIVATE cxx_std_20)
target_link_libraries(bencode_test PRIVATE bittorrent_library)
target_link_libraries(bencode_test PRIVATE Catch2::Catch2WithMain)
add_test(NAME bencode_decode_test COMMAND bencode_test)

add_executable(bencode_encode_test bencode_encode_test.cpp)
target_compile_features(bencode_encode_test PRIVATE cxx_std_20)
target_link_libraries(bencode_encode_test PRIVATE bittorrent_library)
target_link_libraries(bencode_encode_test PRIVATE Catch2::Catch2WithMain)
add_test(NAME bencode_encode_test COMMAND bencode_encode_test)

add_test(NAME cli_torrent_info_test     COMMAND ${CMAKE_SOURCE_DIR}/tests/cli/torrent_info.sh ${CMAKE_BINARY_DIR}/bittorrent ${CMAKE_SOURCE_DIR})
add_test(NAME cli_discover_peers_test   COMMAND ${CMAKE_SOURCE_DIR}/tests/cli/discover_peers.sh ${CMAKE_BINARY_DIR}/bittorrent ${CMAKE_SOURCE_DIR})