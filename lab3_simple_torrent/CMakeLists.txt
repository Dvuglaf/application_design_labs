cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 871ed52d350214a034f6ef8a3b8f51c5ce1bd400) # The commit hash for 1.9.0. Replace with the latest from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)

# utils
include_directories(utils)

add_executable(torrent "torrent.cpp" "torrent_file.h" "utils/bencode/bencode.hpp" "utils/sha1/sha1.h" "utils/sha1/sha1.cpp" 
    "utils/socket/socket.cpp" "utils/socket/socket.h" "peer.h" "utils/utils.h" "bit_message.h" "download_manager.h")
target_link_libraries(torrent PRIVATE cpr::cpr)
target_link_libraries(torrent PRIVATE wsock32 ws2_32)
