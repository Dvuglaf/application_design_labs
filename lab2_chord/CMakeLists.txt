cmake_minimum_required(VERSION 3.5)

add_executable(main main.cpp socket.h socket.cpp chord.h chord.cpp)
add_executable(main1 main1.cpp socket.h socket.cpp chord.h chord.cpp)

if(WIN32)
	target_link_libraries(main wsock32 ws2_32)
	target_link_libraries(main1 wsock32 ws2_32)

endif()