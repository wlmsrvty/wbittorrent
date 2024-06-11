file (GLOB_RECURSE SOURCES
    "*.cpp"
    "*.hpp"
)

add_library(${BITTORRENT_LIB_NAME}
    ${SOURCES})

target_include_directories(${BITTORRENT_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# httplib
Include(FetchContent)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.15.3
)
FetchContent_MakeAvailable(httplib)
target_link_libraries(${BITTORRENT_LIB_NAME} httplib)


find_package(spdlog REQUIRED)
target_link_libraries(${BITTORRENT_LIB_NAME} spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)
