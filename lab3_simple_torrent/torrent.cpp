#include <iostream>
#include <string>
#include <vector>

class bencode_type {
   public:
    virtual bencode_type* get_value() = 0;
    virtual ~bencode_type() = default;
};

class bencode_int : public bencode_type {
    int value;

   public:
    bencode_int get_value override { return value; }
    ~bencode_int() override = default;
};

class bencode_string : public bencode_type {
    std::string value;

   public:
    ~bencode_string() override = default;
};

class bencode_list : public bencode_type {
    std::vector<bencode_type*> value;

   public:
    ~bencode_list() override = default;
};

bencode_int(auto& it) {
    ++it;
    auto start = it;
    while (*it != 'e') {
        ++it;
    }
    return std::stoi(std::string(start, it - 1));
}

std::string bencode_string(auto& it) {
    auto start = it;
    while (*it != ':') {
        ++it;
    }
    size_t len = std::stoull(std::string(start, it - 1));

    ++it;
    start = it;
    for (size_t i = 0; i < len; ++i) {
        ++it;
    }
    return std::string(start, it);
}

void bencode(const std::string& input) {
    for (auto it = input.begin(); it != input.end(); ++it) {
        if (*it == 'i') {
            bencode_int(it);
        } else if (*it == 'l') {
            bencode_list(it);
        } else if (*it == 'd') {
            bencode_dict(it);
        } else if (*it >= '0' && *it <= '9') {
            bencode_string(it);
        }
    }
}

int main() {
    char buff[] = "Hello, world!";
    std::string buffer = std::move(buff);
    std::cout << buffer;
    std::cout << buff[0];
}
