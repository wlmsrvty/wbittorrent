# Testing library
# Add Catch2::Catch2
Include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.4.0 # or a later release
)
FetchContent_MakeAvailable(Catch2)

add_executable(bencode_test bencode_test.cpp)
target_compile_features(bencode_test PRIVATE cxx_std_20)
target_link_libraries(bencode_test PRIVATE ${BITTORRENT_LIB_NAME})
target_link_libraries(bencode_test PRIVATE Catch2::Catch2WithMain)
add_test(NAME bencode_test COMMAND bencode_test)