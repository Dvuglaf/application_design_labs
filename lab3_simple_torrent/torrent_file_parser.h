#pragma once
#include <memory>
#include <string>
#include <vector>
#include "bencode/BItem.h"
#include "bencode/BDictionary.h"

class torrent_file_parser {
private:
    std::shared_ptr<bencoding::BDictionary> root;
public:
    explicit torrent_file_parser(const std::string& path_to_file);
    long get_file_size() const;
    long get_piece_length() const;
    std::string get_file_name() const;
    std::string get_announce() const;
    std::shared_ptr<bencoding::BItem> get(std::string key) const;
    std::string get_info_hash() const;
    std::vector<std::string> split_piece_hashes() const;
};