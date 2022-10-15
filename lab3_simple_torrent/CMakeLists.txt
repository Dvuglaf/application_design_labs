cmake_minimum_required(VERSION 3.5)

add_executable(torrent torrent.cpp)

if(WIN32)
	target_link_libraries(torrent wsock32 ws2_32)
endif()