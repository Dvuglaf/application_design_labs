#pragma once
#include <memory>
#include <string>
#include <vector>
#include "bencode.hpp"

class torrent_file {
public:
    torrent_file(const std::string&);
    torrent_file(const bencode::data&);
    unsigned long long get_file_size() const;
    unsigned long long get_piece_length() const;
    std::string get_file_name() const;
    std::string get_announce() const;
    
    template <typename T>
    T get(std::string key) const;

    std::string get_info_hash() const;
    std::vector<std::string> split_piece_hashes() const;
private:
    bencode::data _data;
};