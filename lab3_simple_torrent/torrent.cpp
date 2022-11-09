#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <random>
#include <memory>
#include <cstdint>
#include <cpr/cpr.h>
#include "utils/bencode/bencode.hpp"
#include "torrent_file.h"
#include "download_manager.h"
#include "utils/utils.h"
#include "peer.h"


class torrent_client {
public:
	torrent_client() : _id(generate_client_id()) { ; }

	void download(const std::string& path_to_file, const std::string& path_to_output_dir) {
		try {
			try {
				_file.add_file(path_to_file);
			}
			catch (...) {
				throw std::invalid_argument("File does not exists or has wrong structure.");
			}

			std::vector<std::shared_ptr<peer>> peers = get_peers();
			std::cout << "get [" << peers.size() << "] peers from tracker\n";

			download_manager manager(peers);

			std::cout << "downloading start\n";
			const std::string file_data = manager.download_pieces(_file.get_info_hash(), _id, 
				_file.get_piece_hashes(), _file.get_piece_length(), _file.get_file_size());
			if (file_data.empty()) {
				return;
			}
			std::cout << "downloading finish\n";

			std::string full_path_with_name = path_to_output_dir + "\\" + _file.get_file_name();
			std::cout << "saving [" << full_path_with_name << "]...\n";

			std::ofstream out;
			out.open(full_path_with_name, std::ios::binary);
			if (out.is_open()) {
				out << file_data << std::endl;
			}
			out.close();
			std::cout << "saved\n";
		}
		catch (std::exception& e) {
			std::cout << e.what();
		}
	}

private:
	struct request_params {
		std::string url;
		std::string client_id;
		std::string info_hash;
		uint64_t uploaded;
		uint64_t downloaded;
		uint64_t left;
		uint16_t port;
	};

private:
	static std::string generate_client_id() {
		std::string peer_id = "-UT2022-";

		// Generate 12 random numbers
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> distrib(1, 9);
		for (int i = 0; i < 12; i++)
			peer_id += std::to_string(distrib(gen));
		return peer_id;
	}

	static std::string get_peers_request(const request_params& params) {
		cpr::Response res = cpr::Get(cpr::Url{ params.url }, cpr::Parameters{
					{ "info_hash", hex_decode(params.info_hash) },
					{ "peer_id", params.client_id },
					{ "port", std::to_string(params.port) },
					{ "uploaded", std::to_string(params.uploaded) },
					{ "downloaded", std::to_string(params.downloaded) },
					{ "left", std::to_string(params.left) },
					{ "compact", std::to_string(1) }
			}, cpr::Timeout{ cpr::Timeout(10000) }
		);
		return res.text;
	}

	std::vector<std::shared_ptr<peer>> get_peers() {
		const std::string announce_url = _file.get_announce();
		const std::string info_hash = _file.get_info_hash();

		request_params params{ announce_url, _id, info_hash,
			0ull, 0ull, _file.get_file_size(), 6881 };

		std::string tracker_response;
		try {
			tracker_response = get_peers_request(params);
			if (tracker_response.empty()) {
				throw;
			}
		}
		catch (...) {
			throw std::runtime_error("Problem with torrent tracker.");
		}

		const auto response_dict = std::get<bencode::dict>(bencode::decode(tracker_response));
		std::vector<std::shared_ptr<peer>> peers;
		if (response_dict->find("peers")->second.index() == 2) {  // peers as bencoded data (list)
			bencode::list peers_list = std::get<bencode::list>(response_dict->find("peers")->second);

			for (auto& p : peers_list) {
				peers.push_back(std::make_shared<peer>(
					std::get<bencode::string>(std::get<bencode::dict>(p)->find("peer id")->second),
					std::get<bencode::string>(std::get<bencode::dict>(p)->find("ip")->second),
					std::get<bencode::integer>(std::get<bencode::dict>(p)->find("port")->second),
					socket_wrapper(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP)
					));
			}
		}
		else {  // peers as raw data
			bencode::string peers_string = std::get<bencode::string>(response_dict->find("peers")->second);
			for (size_t i = 0; i < peers_string.size(); i += 6) {
				std::string ip;
				ip += std::to_string((peers_string[i] + 256) % 256) + ".";
				ip += std::to_string((peers_string[i + 1] + 256) % 256) + ".";
				ip += std::to_string((peers_string[i + 2] + 256) % 256) + ".";
				ip += std::to_string((peers_string[i + 3] + 256) % 256);

				std::string port_data = peers_string.substr(i + 4, 2);
				uint16_t port = get_number_from_raw<uint16_t>(port_data);

				peers.push_back(std::make_shared<peer>(
					_id,
					ip,
					port,
					socket_wrapper(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP)
					));
			}
		}
		return peers;
	}

private:
	std::string _id;
	torrent_file _file;
};


int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cerr << "Arguments: path to .torrent file, output directory to save download file\n";
		return -1;
	}

	torrent_client cl;
	cl.download(argv[1], argv[2]);
}
