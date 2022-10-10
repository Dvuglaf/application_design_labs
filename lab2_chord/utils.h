#ifndef UTILS_LAB_2
#define UTILS_LAB_2

#include <vector>
#include <string>
#include "socket.h"
#include <exception>
#include <iostream>
#include "chord.h"

/*
* Split string by delimiter
*
* @param string input string.
* @param delimiter.
*
* @return array of substrings, splitted by delimiter.
*/
std::vector<std::string> split(const std::string& string, const std::string& delimiter);

/*
* Check if number belongs to interval
*
* @param idx checked number.
* @param start left border of interval.
* @param end right border of interval.
* @param left_included to include left border or not.
* @param right_included to include right border or not.
*
* @return whether the number belongs to the interval.
*/
bool does_belong(unsigned long idx, unsigned long start, unsigned long end, bool left_included, bool right_included);

/*
* Receive string via socket
*
* @param slave_socket receiving socket.
*
* @return received buffer with type std::string.
*/
std::string receive_via_socket(const socket_wrapper& slave_socket);

/*
* Check user enter: id, ip, port
*
* @param id id to check [0..2^m - 1].
* @param ip ip to check.
* @param port port to check [0..65535].
* 
* @return received buffer with type std::string.
*/
void check_users_params(unsigned long id, const std::string& ip, unsigned long port);

#endif // !UTILS_LAB_2