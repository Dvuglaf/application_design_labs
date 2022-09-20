cmake_minimum_required(VERSION 3.5)

set(BUILD_SHARED_LIBS OFF)
find_package(OpenCV REQUIRED)

# add_executable(lab1_client_server client.cpp socket.h socket.cpp utils.h utils.cpp) # uncomment for client
# add_executable(lab1_client_server server.cpp socket.h socket.cpp utils.h utils.cpp) # uncomment for server

if(WIN32)
 target_link_libraries(lab1_client_server wsock32 ws2_32)
 target_link_libraries(lab1_client_server ${OpenCV_LIBS} )
endif()