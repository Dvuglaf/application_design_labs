#ifndef _TORRENT_FILE_H
#define _TORRENT_FILE_H
#include <string>
#include <vector>
#include <fstream>
#include "utils/bencode/bencode.hpp"
#include "utils/sha1/sha1.h"

class torrent_file {
public:
    torrent_file() = default;

    torrent_file(const std::string& path_to_file) {
        std::ifstream file_stream(path_to_file, std::ifstream::binary);
        _data = bencode::decode(file_stream);
    }

    torrent_file(const bencode::data& data) : _data(data) {}

    void add_file(const std::string& path_to_file) {
        std::ifstream file_stream(path_to_file, std::ifstream::binary);
        _data = bencode::decode(file_stream);
    }

    bencode::dict get_info_dict() const {
        const auto dict = std::get<bencode::dict>(_data);
        return std::get<bencode::dict>(dict->find("info")->second);
    }

    std::string get_info_hash() const {
        const auto info_dictionary = get_info_dict();
        if (info_dictionary.empty())
            throw std::runtime_error("Torrent file is malformed. [File does not contain key 'info']");

        std::string info_string = bencode::encode(info_dictionary);
        std::string sha1_hash = sha1(info_string);
        return sha1_hash;
    }

    std::vector<std::string> get_piece_hashes() const {
        const auto info_dictionary = get_info_dict();

        const auto pieces = std::get<bencode::string>(info_dictionary->find("pieces")->second);
        if (pieces.empty())
            throw std::runtime_error("Torrent file is malformed. [File does not contain key 'pieces']");

        std::vector<std::string> piece_hashes;

        uint32_t pieces_count = pieces.size() / _HASH_LEN;
        piece_hashes.reserve(pieces_count);
        for (uint32_t i = 0; i < pieces_count; i++)
            piece_hashes.push_back(pieces.substr(i * _HASH_LEN, _HASH_LEN));
        return piece_hashes;
    }

    uint64_t get_file_size() const {
        const auto info_dictionary = get_info_dict();
        if (info_dictionary->find("length") != info_dictionary.end()) {  // only one file with givven length
            const auto file_size = std::get<bencode::integer>(info_dictionary->find("length")->second);
            return file_size;
        }
        else {
            throw std::runtime_error("Torrent file is malformed. [File does not contain any information about file size or more than one file]");
        }
        const auto file_size = std::get<bencode::integer>(info_dictionary->find("length")->second);

        return file_size;
    }

    uint32_t get_piece_length() const {
        const auto info_dictionary = get_info_dict();
        const auto piece_length = std::get<bencode::integer>(info_dictionary->find("piece length")->second);

        if (!piece_length)
            throw std::runtime_error("Torrent file is malformed. [File does not contain key 'piece length']");
        return static_cast<uint32_t>(piece_length);
    }

    std::string get_file_name() const {
        const auto info_dictionary = get_info_dict();
        const auto file_name = std::get<bencode::string>(info_dictionary->find("name")->second);
        if (file_name.empty())
            throw std::runtime_error("Torrent file is malformed. [File does not contain key 'name']");
        return file_name;
    }

    std::string get_announce() const {
        const auto dict = std::get<bencode::dict>(_data);
        const auto announce = std::get<bencode::string>(dict->find("announce")->second);;
        if (announce.empty())
            throw std::runtime_error("Torrent file is malformed. [File does not contain key 'announce']");
        return announce;
    }

private:
    bencode::data _data;
    const size_t _HASH_LEN = 20;
};
#endif