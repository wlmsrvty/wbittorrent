file (GLOB_RECURSE SOURCES
    "*.cpp"
    "*.hpp"
)

add_library(bittorrent_library
    ${SOURCES})

target_include_directories(bittorrent_library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# httplib
Include(FetchContent)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.15.3
)
FetchContent_MakeAvailable(httplib)
target_link_libraries(bittorrent_library httplib)


Include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.x
)
FetchContent_MakeAvailable(spdlog)

target_link_libraries(bittorrent_library spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)
