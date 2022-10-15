#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <map>

template<typename T>
struct string_less {
	bool operator()(const T& left, const T& right) const {
		std::string first = std::get<std::string>(left._value);
		std::string second = std::get<std::string>(right._value);
		return first.size() < second.size();
	}
};

struct element {
	enum element_type {
		bencode_int,
		bencode_string,
		bencode_list,
		bencode_dict
	};

	element_type _type;
	std::variant<int, std::string, std::vector<element>, std::map<element, element, string_less<element>>> _value;

	element(element_type t, const
		std::variant<int, std::string, std::vector<element>, std::map<element, element, string_less<element>>>& value)
		: _type(t), _value(value) {}
};

std::ostream& operator<<(std::ostream& out, const element& value);

std::ostream& operator<<(std::ostream& out, const std::vector<element>& value) {
	out << "[";
	for (auto& it : value) {
		out << it << ", ";
	}
	out << "]";
	return out;
}

std::ostream& operator<<(std::ostream& out, const std::map<element, element, string_less<element>>& value) {
	out << "{";
	for (auto& it : value) {
		out << it.first << ": " << it.second << ", " << std::endl;
	}
	out << "}";
	return out;
}

std::ostream& operator<<(std::ostream& out, const element& value) {
	if (value._type == element::bencode_int) {
		out << std::get<int>(value._value);
	} else if (value._type == element::bencode_list) {
		out << std::get<std::vector<element>>(value._value);
	} else if (value._type == element::bencode_string) {
		out << std::get<std::string>(value._value);
	} else if (value._type == element::bencode_dict) {
		out << std::get<std::map<element, element, string_less<element>>>(value._value);
	}
	return out;
}

element get_int(auto& it) {
	auto start = it;
	while (*it != 'e') {
		++it;
	}
	return element(element::bencode_int, std::stoi(std::string(start, it++)));
}

element get_string(auto& it) {
	auto start = it;
	while (*it != ':') {
		++it;
	}
	size_t len = std::stoull(std::string(start, it));

	++it;
	start = it;

	for (size_t i = 0; i < len; ++i) {
		++it;
	}
	return element(element::bencode_string, std::string(start, it--));
}

element get_element(std::string::const_iterator& it) {
	if (*it == 'i') {
		return get_int(++it);
	}
	else if (*it == 'l') {
		std::vector<element> list;
		while (*it != 'e') {
			list.push_back(get_element(++it));
			++it;
		}
		return element(element::element_type::bencode_list, list);
	}
	else if (*it == 'd') {
		std::map<element, element, string_less<element>> dict;
		while (*it != 'e') {
			const element key = get_string(++it);
			element value = get_element(++it);
			dict.emplace(key, value);
		}
		return element(element::element_type::bencode_dict, dict);
	}
	else if (*it >= '0' && *it <= '9') {
		return get_string(it);
	}
	else
		throw std::invalid_argument("Invalid input string!");
}

std::vector<element> read_bencode(const std::string& input) {
	std::vector<element> elements;

	for (auto it = input.begin(); it != input.end(); ++it) {
		elements.push_back(get_element(it));
	}

	return elements;
}

int main() {
	std::string input = "d3:bar4:spam4:foooi42ee";
	auto result = read_bencode(input);
	for (auto& it : result) {
		std::cout << it << std::endl;
	}
}
