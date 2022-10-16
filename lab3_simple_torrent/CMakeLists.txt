cmake_minimum_required(VERSION 3.12)

add_executable(torrent torrent.cpp bencode.h bencode.cpp SHA1.h SHA1.cpp utils.h utils.cpp socket.cpp socket.h)

if(WIN32)
	target_link_libraries(torrent wsock32 ws2_32)
endif()

set_property(TARGET torrent PROPERTY CXX_STANDARD 20)
