#include "bencode.hpp"
#include "error.hpp"
#include "lib/nlohmann/json.hpp"
#include "lib/nonstd/expected.hpp"
#include "peer.hpp"
#include "torrent.hpp"
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

using json = nlohmann::json;

static int torrent_info(std::string const& file_path) {
    std::error_code ec;
    auto get_torrent = bittorrent::Torrent::parse_torrent(file_path, ec);
    if (get_torrent.has_value() == false) {
        std::cerr << "Error parsing torrent: " << ec << std::endl;
        return 1;
    }
    auto torrent = get_torrent.value();
    std::cout << "Tracker URL: " << torrent.announce << std::endl;
    std::cout << "Length: " << torrent.length << std::endl;
    std::cout << "Info Hash: " << torrent.info_hash() << std::endl;

    std::cout << "Piece Length: " << torrent.piece_length << std::endl;

    std::cout << "Piece Hashes:" << std::endl;
    auto piece_hashes = torrent.piece_hashes();
    auto cout_flags = std::cout.flags();
    for (std::string const& hash : piece_hashes) {
        for (unsigned char c : hash)
            std::cout << std::setw(2) << std::setfill('0') << std::hex
                      << static_cast<unsigned int>(c);
        std::cout << std::endl;
        std::cout.flags(cout_flags);
    }
    return 0;
}

static int discover_peers(std::string const& file_path) {
    std::error_code ec;
    auto torrent = bittorrent::Torrent::parse_torrent(file_path, ec);
    if (torrent.has_value() == false) {
        std::cerr << "Error parsing torrent: " << ec << std::endl;
        return 1;
    }
    auto tracker_info = torrent.value().discover_peers(ec);
    if (ec) {
        std::cerr << "Error discovering peers: " << ec << std::endl;
        return 1;
    }
    return 0;
}

static int handshake(std::string const& torrent_path, std::string const& peer) {
    std::error_code ec;
    auto get_torrent = bittorrent::Torrent::parse_torrent(torrent_path, ec);
    if (get_torrent.has_value() == false) {
        std::cerr << "Error parsing torrent: " << ec << std::endl;
        return 1;
    }
    auto torrent = get_torrent.value();
    std::string peer_ip = peer.substr(0, peer.find(':'));
    std::string peer_port_str = peer.substr(peer.find(':') + 1);
    uint16_t peer_port = std::stoi(peer_port_str);

    bittorrent::Peer p(peer_ip, peer_port);
    p.establish_connection();
    p.handshake(torrent.info_hash_raw());
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " decode <encoded_value>"
                  << std::endl;
        return 1;
    }

    std::string command = argv[1];

    if (command == "decode") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>"
                      << std::endl;
            return 1;
        }

        std::string encoded_value = argv[2];
        std::error_code ec;
        auto decoded_value =
            bittorrent::Bencode::decode_bencoded_value(encoded_value, ec);

        if (ec) {
            std::cerr << "Error decoding bencoded value: " << ec << std::endl;
            return 1;
        }
        std::cout << decoded_value.dump() << std::endl;
    }

    else if (command == "info") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " info <torrent file>"
                      << std::endl;
            return 1;
        }
        return torrent_info(argv[2]);
    }

    else if (command == "peers") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " peers <torrent file>"
                      << std::endl;
            return 1;
        }
        return discover_peers(argv[2]);
    }

    else if (command == "handshake") {
        if (argc < 4) {
            std::cerr << "Usage: " << argv[0]
                      << " handshake <torrent file> <peer_ip>:<peer_port>"
                      << std::endl;
            return 1;
        }
        return handshake(argv[2], argv[3]);
    }

    else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
