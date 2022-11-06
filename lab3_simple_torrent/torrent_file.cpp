#include <regex>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include "bencode.hpp"
#include "sha1/sha1.h"
#include "torrent_file.h"
#include <memory>


#define HASH_LEN 20


torrent_file::torrent_file(const std::string& path_to_file) {
    std::ifstream file_stream(path_to_file, std::ifstream::binary);
    _data = bencode::decode(file_stream);
}

torrent_file::torrent_file(const bencode::data& data) : _data(data) {}

template <typename T>
T torrent_file::get(std::string key) const {
    const auto dict = std::get<bencode::dict>(_data);
    return std::get<T>(dict->find(key)->second);
}

std::string torrent_file::get_info_hash() const {
    const auto info_dictionary = get<bencode::dict>("info");
    if (info_dictionary.empty())
        throw std::runtime_error("Torrent file is malformed. [File does not contain key 'info']");

    std::string info_string = bencode::encode(info_dictionary);
    std::string sha1_hash = sha1(info_string);
    return sha1_hash;
}

std::vector<std::string> torrent_file::split_piece_hashes() const {
    const auto info_dictionary = get<bencode::dict>("info");

    const auto pieces = std::get<bencode::string>(info_dictionary->find("pieces")->second);
    if (pieces.empty())
        throw std::runtime_error("Torrent file is malformed. [File does not contain key 'pieces']");

    std::vector<std::string> piece_hashes;
    assert(pieces.size() % HASH_LEN == 0);

    int pieces_count = (int) pieces.size() / HASH_LEN;
    piece_hashes.reserve(pieces_count);
    for (int i = 0; i < pieces_count; i++)
        piece_hashes.push_back(pieces.substr(i * HASH_LEN, HASH_LEN));
    return piece_hashes;
}

unsigned long long torrent_file::get_file_size() const {
    const auto info_dictionary = get<bencode::dict>("info");
    const auto file_size = std::get<bencode::integer>(info_dictionary->find("length")->second);

    if (!file_size)
        throw std::runtime_error("Torrent file is malformed. [File does not contain key 'length']");
    return file_size;
}

unsigned long long torrent_file::get_piece_length() const {
    const auto info_dictionary = get<bencode::dict>("info");
    const auto piece_length = std::get<bencode::integer>(info_dictionary->find("piece length")->second);

    if (!piece_length)
        throw std::runtime_error("Torrent file is malformed. [File does not contain key 'piece length']");
    return piece_length;
}

std::string torrent_file::get_file_name() const {
    const auto file_name = get<bencode::string>("name");
    if (file_name.empty())
        throw std::runtime_error("Torrent file is malformed. [File does not contain key 'name']");
    return file_name;
}

std::string torrent_file::get_announce() const {
    const auto announce = get<bencode::string>("announce");
    if (announce.empty())
        throw std::runtime_error("Torrent file is malformed. [File does not contain key 'announce']");
    return announce;
}