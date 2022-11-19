cmake_minimum_required(VERSION 3.5)

set(BUILD_SHARED_LIBS OFF)
find_package(OpenCV REQUIRED)

# utils
include_directories(utils)

add_executable(server server.cpp "utils/socket/socket.cpp" "utils/socket/socket.h" "utils/utils.hpp")
add_executable(repeater repeater.cpp "utils/socket/socket.cpp" "utils/socket/socket.h" "utils/utils.hpp")
add_executable(client client.cpp "utils/socket/socket.cpp" "utils/socket/socket.h" "utils/utils.hpp")



if(WIN32)
	target_link_libraries(server wsock32 ws2_32)
	target_link_libraries(server ${OpenCV_LIBS} )

	target_link_libraries(repeater wsock32 ws2_32)
	target_link_libraries(repeater ${OpenCV_LIBS} )

	target_link_libraries(client wsock32 ws2_32)
	target_link_libraries(client ${OpenCV_LIBS} )
endif()