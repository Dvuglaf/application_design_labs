#ifndef _BIT_MESSAGE_H
#define _BIT_MESSAGE_H
#include <string>
#include <cstdint>

class bit_message {
public:
	enum message_id {
		keep_alive = -1,
		choke = 0,
		unchoke = 1,
		interested = 2,
		not_interested = 3,
		have = 4,
		bit_field = 5,
		request = 6,
		piece = 7
	};

public:
	bit_message(const message_id id, const std::string& payload = "") : _id(id), _payload(payload) { ; }

	static std::string create_message(const uint32_t length, const uint8_t id, const std::string& payload) {
		auto message_length = htonl(length);

		char temp[5];

		std::memcpy(temp, &message_length, sizeof(message_length));
		std::memcpy(temp + 4, &id, sizeof(id));
		std::string buffer;
		for (size_t i = 0; i < 5; ++i)
			buffer += (char)temp[i];

		return buffer + payload;
	}

	int8_t get_id() const noexcept { return _id; }
	std::string get_payload() const noexcept { return _payload; }

private:
	int8_t _id;
	std::string _payload;
};
#endif