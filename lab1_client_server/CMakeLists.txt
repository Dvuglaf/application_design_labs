cmake_minimum_required(VERSION 3.5)

set(BUILD_SHARED_LIBS OFF)
find_package(OpenCV REQUIRED)

# add_executable(client client.cpp socket.h socket.cpp utils.h utils.cpp)
# add_executable(server server.cpp socket.h socket.cpp utils.h utils.cpp)

# if(WIN32)
#  target_link_libraries(client wsock32 ws2_32)
#  target_link_libraries(client ${OpenCV_LIBS} )

#  target_link_libraries(server wsock32 ws2_32)
#  target_link_libraries(server ${OpenCV_LIBS} )
# endif()