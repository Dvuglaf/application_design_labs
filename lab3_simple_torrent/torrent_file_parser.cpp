#include <regex>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include "bencode/BItem.h"
#include "bencode/Decoder.h"
#include "bencode/BDictionary.h"
#include "bencode/Encoder.h"
#include "bencode/bencoding.h"
#include "sha1/sha1.h"
#include "torrent_file_parser.h"
#include <memory>


#define HASH_LEN 20


torrent_file_parser::torrent_file_parser(const std::string& path_to_file) {
    std::ifstream file_stream(path_to_file, std::ifstream::binary);
    std::shared_ptr<bencoding::BItem> decoded_torrent_file = bencoding::decode(file_stream);
    std::shared_ptr<bencoding::BDictionary> root_dict =
        std::dynamic_pointer_cast<bencoding::BDictionary>(decoded_torrent_file);
    root = root_dict;
}

std::shared_ptr<bencoding::BItem> torrent_file_parser::get(std::string key) const {
    std::shared_ptr<bencoding::BItem> value = root->getValue(key);
    return value;
}

std::string torrent_file_parser::get_info_hash() const {
    std::shared_ptr<bencoding::BItem> info_dictionary = get("info");
    std::string info_string = bencoding::encode(info_dictionary);
    std::string sha1_hash = sha1(info_string);
    return sha1_hash;
}

std::vector<std::string> torrent_file_parser::split_piece_hashes() const {
    std::shared_ptr<bencoding::BItem> pieces_value = get("pieces");
    if (!pieces_value)
        throw std::runtime_error("Torrent file is malformed. [File does not contain key 'pieces']");
    std::string pieces = std::dynamic_pointer_cast<bencoding::BString>(pieces_value)->value();

    std::vector<std::string> piece_hashes;
    assert(pieces.size() % HASH_LEN == 0);

    int pieces_count = (int) pieces.size() / HASH_LEN;
    piece_hashes.reserve(pieces_count);
    for (int i = 0; i < pieces_count; i++)
        piece_hashes.push_back(pieces.substr(i * HASH_LEN, HASH_LEN));
    return piece_hashes;
}

long torrent_file_parser::get_file_size() const {
    std::shared_ptr<bencoding::BItem> file_size_item = get("length");
    if (!file_size_item)
        throw std::runtime_error("Torrent file is malformed. [File does not contain key 'length']");
    long file_size = std::dynamic_pointer_cast<bencoding::BInteger>(file_size_item)->value();
    return file_size;
}

long torrent_file_parser::get_piece_length() const {
    std::shared_ptr<bencoding::BItem> piece_length_item = get("piece length");
    if (!piece_length_item)
        throw std::runtime_error("Torrent file is malformed. [File does not contain key 'piece length']");
    long piece_length = std::dynamic_pointer_cast<bencoding::BInteger>(piece_length_item)->value();
    return piece_length;
}

std::string torrent_file_parser::get_file_name() const
{
    std::shared_ptr<bencoding::BItem> file_name_item = get("name");
    if (!file_name_item)
        throw std::runtime_error("Torrent file is malformed. [File does not contain key 'name']");
    std::string file_name = std::dynamic_pointer_cast<bencoding::BString>(file_name_item)->value();
    return file_name;
}

std::string torrent_file_parser::get_announce() const
{
    std::shared_ptr<bencoding::BItem> announce_item = get("announce");
    if (!announce_item)
        throw std::runtime_error("Torrent file is malformed. [File does not contain key 'announce']");
    std::string announce = std::dynamic_pointer_cast<bencoding::BString>(announce_item)->value();
    return announce;
}