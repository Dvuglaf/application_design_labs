cmake_minimum_required(VERSION 3.5)

add_executable(chord main.cpp socket.h socket.cpp chord.h chord.cpp utils.h utils.cpp)

if(WIN32)
	target_link_libraries(chord wsock32 ws2_32)
endif()