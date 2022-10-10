cmake_minimum_required(VERSION 3.5)

add_executable(main main.cpp socket.h socket.cpp chord.h chord.cpp utils.h utils.cpp)
add_executable(main_debug main_debug.cpp socket.h socket.cpp chord.h chord.cpp utils.h utils.cpp)

if(WIN32)
	target_link_libraries(main wsock32 ws2_32)
	target_link_libraries(main_debug wsock32 ws2_32)
endif()