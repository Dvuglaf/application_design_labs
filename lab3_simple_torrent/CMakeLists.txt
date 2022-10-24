cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 871ed52d350214a034f6ef8a3b8f51c5ce1bd400) # The commit hash for 1.9.0. Replace with the latest from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)

# bencoding
include_directories(bencode)
#file(GLOB BENCODING_SRC "bencode/*.cpp" "bencode/*.h")
#add_library(bencoding STATIC ${BENCODING_SRC})

# sha1
include_directories(sha1)
#add_library(sha1 STATIC sha1/sha1.h sha1/sha1.cpp)

add_executable(torrent torrent.cpp torrent_file_parser.h torrent_file_parser.cpp "sha1/sha1.h" "sha1/sha1.cpp" "bencode/BDictionary.cpp" "bencode/BDictionary.h" "bencode/bencoding.h" "bencode/BInteger.h" "bencode/BInteger.cpp" "bencode/BItem.h" "bencode/BItem.cpp" "bencode/BItemVisitor.h" "bencode/BItemVisitor.cpp" "bencode/BList.cpp" "bencode/BList.h" "bencode/BString.h" "bencode/BString.cpp" "bencode/Decoder.cpp" "bencode/Decoder.h" "bencode/Encoder.cpp" "bencode/Encoder.h" "bencode/PrettyPrinter.cpp" "bencode/PrettyPrinter.h" "bencode/Utils.h" "bencode/Utils.cpp" )
target_link_libraries(torrent PRIVATE cpr::cpr)