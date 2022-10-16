#include "bencode.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <variant>


bencode_element::bencode_element(const __types_variant& value) : _value(value), _index(value.index()) {}

size_t bencode_element::get_type() const noexcept { return _index; }

const bencode_element::__types_variant& bencode_element::get_value() const noexcept { return _value; }

std::ostream& operator<<(std::ostream& out, const bencode_element& elem) {
	if (elem.get_type() == 0) {
		out << std::get<int_type>(elem.get_value());
	}
	else if (elem.get_type() == 1) {
		out << std::get<ustring_type>(elem.get_value());
	}
	else if (elem.get_type() == 2) {
		out << std::get<list_type>(elem.get_value());
	}
	else if (elem.get_type() == 3) {
		out << std::get<dict_type>(elem.get_value());
	}
	return out;
}

std::ostream& operator<<(std::ostream& out, const ustring_type& string) {
	for (auto& it : string) {
		out << it;
	}
	return out;
}

std::ostream& operator<<(std::ostream& out, const list_type& list) {
	out << "[";
	for (auto& it : list) {
		out << it << ",";
	}
	out << "]";
	return out;
}

std::ostream& operator<<(std::ostream& out, const dict_type& dict) {
	out << "\n\t{\n";
	for (auto& it : dict) {
		out << "\t" << it.first << ": " << it.second << ",\n";
	}
	out << "\n\t}";
	return out;
}

bencode_element get_int(ustring_type::const_iterator& it) {
	const auto start = it;
	while (*it != 'e') {  // int value has end symbol 'e'
		++it;
	}
	return bencode_element(std::stoi(std::string(start, it)));  // it on 'e'
}

bencode_element get_string(ustring_type::const_iterator& it) {
	const auto start_len = it;
	while (*it != ':') {
		++it;
	}
	size_t len = std::stoull(std::string(start_len, it));  // it on ':'
	if (len == 0) {
		return bencode_element(ustring_type());
	}

	++it;
	const auto start_value = it;

	for (size_t i = 0; i < len - 1; ++i) {
		++it;
	}
	return bencode_element(ustring_type(start_value, it + 1));  // since right border not included
}

bencode_element get_bencode_element(ustring_type::const_iterator& it) {
	if (*it == 'i') {  // integer
		auto value = get_int(++it);
		return value;
	}
	else if (*it == 'l') {  // list of different types
		++it;
		list_type list;
		while (*it != 'e') {
			list.push_back(get_bencode_element(it));
			++it;
		}
		return bencode_element(list);
	}
	else if (*it == 'd') {  // dictionary with key - const string, value - any bencode type"
		++it;
		dict_type dict;
		while (*it != 'e') {
			const ustring_type key = 
				std::get<ustring_type>(get_string(it).get_value());
			++it;
			bencode_element value = get_bencode_element(it);
			++it;
			dict.emplace(key, value);
		}
		return bencode_element(dict);
	}
	else if (*it >= '0' && *it <= '9') {
		bencode_element value = get_string(it);
		return value;
	}
	else
		throw std::invalid_argument("Invalid input string!");
}

list_type read_bencode(const ustring_type& input) {
	list_type bencode_elements;

	for (auto it = input.begin(); it != input.end(); ++it) {
		bencode_elements.push_back(get_bencode_element(it));
	}

	return bencode_elements;
}
