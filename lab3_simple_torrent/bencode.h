#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <variant>

// Lexicographic order of string types keys in map
struct string_comparator {
	bool operator()(const std::basic_string<unsigned char>& left,
		const std::basic_string<unsigned char>& right) const 
	{
		int result = left.compare(right);
		return result < 0;
	}
};

class bencode_element;

using int_type = int;
using ustring_type = std::basic_string<unsigned char>;
using list_type = std::vector<bencode_element>;
using dict_type = std::map<ustring_type, bencode_element, string_comparator>;

class bencode_element {
private:
	using __types_variant = std::variant<
		int,
		ustring_type,
		list_type,
		dict_type
	>;

public:
	bencode_element(const __types_variant&);

	size_t get_type() const noexcept;
	const __types_variant& get_value() const noexcept;

private:
	__types_variant _value;
	size_t _index;
};

std::ostream& operator<<(std::ostream&, const bencode_element&);
std::ostream& operator<<(std::ostream&, const ustring_type&);
std::ostream& operator<<(std::ostream&, const list_type&);
std::ostream& operator<<(std::ostream&, const dict_type&);

/*
* After each call of get function, the iterator needs to be incremented from the outside;
* since the iterator is located on the last character that belongs to this data type.
*/

bencode_element get_int(ustring_type::const_iterator&);
bencode_element get_string(ustring_type::const_iterator&);
bencode_element get_bencode_element(ustring_type::const_iterator&);

list_type read_bencode(const ustring_type&);