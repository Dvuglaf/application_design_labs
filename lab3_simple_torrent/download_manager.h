#include <string>
#include <vector>
#include <cstdint>
#include <thread>
#include <memory>
#include <iomanip>
#include <chrono>
#include "utils/sha1/sha1.h"
#include "utils/utils.h"
#include "peer.h"


class download_manager {
public:
	download_manager() = delete;

	download_manager(const std::vector<std::shared_ptr<peer>>& peers) {
		for (const auto& p : peers) {
			_peers.push_back(p);
		}
	}

	uint32_t get_next_download_index(const std::shared_ptr<peer>& p) {
		_mutex.lock();
		for (uint32_t i = 0; i < _pieces.size(); ++i) {
			if ((_pieces[i].status == "not downloaded") && p->get_bitfield()[i] == '1') {
				set_piece_status(i, "downloading");
				_mutex.unlock();
				return i;
			}
		}
		_mutex.unlock();

		return ULONG_MAX;
	}

	void set_piece_status(const size_t index, const std::string& new_status) {
		_pieces[index].status = new_status;
	}

	void peer_download_pieces(const std::shared_ptr<peer>& p, const std::string& info_hash, const std::string& client_id) {
		while (!_is_complete.load()) {
			uint32_t index = ULONG_MAX;
			try {
				p->establish_peer_connection({ info_hash, client_id });
				p->send_interested();

				while (!_is_complete.load()) {

					bit_message message = p->receive_message();

					if (message.get_id() == bit_message::keep_alive) {
						p->set_status("keep alive");
						p->send_interested();
						continue;
					}

					if (message.get_id() == bit_message::choke) {
						p->set_status("choked");
						continue;
					}
					else if (message.get_id() == bit_message::unchoke) {
						p->set_status("unchoked");  // start request pieces
					}
					else if (message.get_id() == bit_message::have) {
						p->update_bitfield_from_have_message(message);
						p->send_interested();
						continue;
					}
					else if (message.get_id() == bit_message::piece) {
						p->set_status("received piece");
						std::string data = message.get_payload().substr(8);  // index + begin + data
						if (hex_decode(sha1(data)) != _pieces[index].hash) {
							_mutex.lock();
							set_piece_status(index, "not downloaded");
							_mutex.unlock();
						}
						else {
							set_piece_status(index, "downloaded");
							++_piece_downloaded;
							_pieces[index].data = data;

							if (_piece_downloaded.load() == _pieces.size()) {
								_is_complete.store(true);
								return;
							}
						}
					}
					index = get_next_download_index(p);

					if (index == ULONG_MAX)
						continue;

					p->request_piece(index, _piece_length, static_cast<uint32_t>(_pieces.size()), _file_size);
					p->set_status("requested piece");
				}

			}
			catch (wrong_connection& e) {
				p->set_status("");
				_mutex.lock();
				set_piece_status(index, "not downloaded");
				_mutex.unlock();
				std::cout << e.what() << std::endl;

				using namespace std::chrono_literals;
				std::this_thread::sleep_for(10000ms);
				continue;
			}
			catch (std::exception& e) {
				_mutex.lock();
				set_piece_status(index, "not downloaded");
				_mutex.unlock();
				p->set_status("");
				std::cout << e.what() << ": " << "one peer disconnected\n";
				return;
			}
		}
	}

	uint32_t get_count_active_peers() {
		uint32_t num = 0;
		for (const auto& p : _peers) {
			if (p->get_status() == "requested piece")
				++num;
		}
		return num;
	}

	std::string download_pieces(const std::string& info_hash, const std::string& client_id, 
		const std::vector<std::string>& piece_hashes, const uint32_t piece_length, const uint64_t file_size) 
	{
		_piece_length = piece_length;
		_file_size = file_size;

		for (size_t i = 0; i < piece_hashes.size(); ++i) {
			_pieces.push_back({ "", piece_hashes[i], "not downloaded" });
		}

		time_t start_time = std::time(nullptr);
		time_t current_time = std::time(nullptr);
		double diff = std::difftime(current_time, start_time);

		for (size_t i = 0; i < std::min<size_t>(std::thread::hardware_concurrency(), _peers.size()); ++i) {  // create threads
			std::thread peer_thread(
				&download_manager::peer_download_pieces, this, std::cref(_peers[i]), std::cref(info_hash), std::cref(client_id)
			);
			peer_thread.detach();
		}

		while (!_is_complete.load()) {
			current_time = std::time(nullptr);
			diff = std::difftime(current_time, start_time);

			auto percent = (float)_piece_downloaded.load() / _pieces.size() * 100.f;

			std::cout << "peers [" << get_count_active_peers() << "/" << _peers.size() << "]"
				<< " downloading [" << std::to_string(_piece_downloaded.load()) << "/" << _pieces.size() << "] "
				<< percent << std::setprecision(percent < 10.f ? 3 : 4) << "% in " << diff << "s\r";

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1000ms);

			if (_piece_downloaded.load() == 0 && diff > 200.) {
				std::cout << "Can't download file, problem with peers...";
				return "";
			}
		}
		std::cout << "\n";

		std::string data;
		for (size_t i = 0; i < _pieces.size(); ++i) {
			data += _pieces[i].data;
		}

		return data;
	}

private:
	struct piece {
		std::string data;
		std::string hash;
		std::string status;
	};
private:
	std::vector<std::shared_ptr<peer>> _peers;
	std::vector<piece> _pieces;
	uint32_t _piece_length = 0;
	uint64_t _file_size = 0;

	std::atomic<uint32_t> _piece_downloaded = 0;
	std::atomic<bool> _is_complete = false;
	std::mutex _mutex;
};