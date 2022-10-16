#ifndef UTILS_LAB_3
#define UTILS_LAB_3

#include <vector>
#include <string>
#include "socket.h"
#include <exception>
#include <iostream>

/*
* Split string by delimiter.
*
* @param string: input string
* @param delimiter: delimiter
*
* @return array of substrings, splitted by delimiter
*/
std::vector<std::basic_string<unsigned char>> split(const std::basic_string<unsigned char>& string, const unsigned char delimiter);

/*
* Receive string via socket.
*
* @param slave_socket: receiving socket
*
* @return received buffer with type std::string
*/
std::string receive_via_socket(const socket_wrapper& slave_socket);

#endif // !UTILS_LAB_3