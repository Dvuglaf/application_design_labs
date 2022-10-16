#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <variant>

// Lexicographic order of strings keys in map
struct string_comparator {
	bool operator()(const std::string& left, const std::string& right) const {
		int result = left.compare(right);
		return result < 0;
	}
};

class bencode_element {
public:
	using int_type = int;
	using string_type = std::string;
	using list_type = std::vector<bencode_element>;
	using dict_type = std::map<string_type, bencode_element, string_comparator>;

private:
	using __types_variant = std::variant<
		int,
		string_type,
		list_type,
		dict_type
	>;

public:
	bencode_element(const __types_variant&);

	size_t get_type() const noexcept;
	const auto& get_value() const noexcept;

private:
	__types_variant _value;
	size_t _index;
};

std::ostream& operator<<(std::ostream&, const bencode_element&);
std::ostream& operator<<(std::ostream&, const bencode_element::list_type&);
std::ostream& operator<<(std::ostream&, const bencode_element::dict_type&);

/*
* After each call of get function, the iterator needs to be incremented from the outside;
* since the iterator is located on the last character that belongs to this data type.
*/

bencode_element get_int(std::string::const_iterator&);
bencode_element get_string(std::string::const_iterator&);
bencode_element get_bencode_element(std::string::const_iterator&);

bencode_element::list_type read_bencode(const std::string&);