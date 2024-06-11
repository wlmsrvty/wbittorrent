#include "bencode.hpp"
#include "error.hpp"
#include "lib/nlohmann/json.hpp"
#include "peer.hpp"
#include "spdlog/spdlog.h"
#include "torrent.hpp"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

using json = nlohmann::json;

static int torrent_info(std::string const& file_path) {
    std::error_code ec;
    auto get_torrent = bittorrent::Torrent::parse_torrent(file_path, ec);
    if (!get_torrent) {
        std::cerr << "Error parsing torrent: " << ec << std::endl;
        return 1;
    }
    bittorrent::Torrent const& torrent = *get_torrent;
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
    if (!torrent) {
        std::cerr << "Error parsing torrent: " << ec << std::endl;
        return 1;
    }
    auto get_tracker_info = torrent->discover_peers(ec);
    bittorrent::TrackerInfo tracker_info = get_tracker_info.value();
    for (std::string const& peer : tracker_info.peers) {
        std::cout << peer << std::endl;
    }
    if (ec) {
        std::cerr << "Error discovering peers: " << ec << std::endl;
        return 1;
    }
    return 0;
}

static int handshake(std::string const& torrent_path, std::string const& peer) {
    std::error_code ec;
    auto get_torrent = bittorrent::Torrent::parse_torrent(torrent_path, ec);
    if (!get_torrent) {
        std::cerr << "Error parsing torrent: " << ec << std::endl;
        return 1;
    }
    bittorrent::Torrent const& torrent = *get_torrent;
    std::string peer_ip = peer.substr(0, peer.find(':'));
    std::string peer_port_str = peer.substr(peer.find(':') + 1);
    uint16_t peer_port = std::stoi(peer_port_str);

    bittorrent::Peer p(peer_ip, peer_port, torrent);
    ec = p.establish_connection();
    if (ec) {
        std::cerr << "Error establishing connection with peer: " << ec
                  << std::endl;
        return 1;
    }

    ec = p.handshake(torrent.info_hash_raw());
    if (ec) {
        std::cerr << "Error handshaking with peer: " << ec << std::endl;
        return 1;
    }

    std::cout << "Peer ID: " << p.peer_id << std::endl;

    return 0;
}

static std::optional<std::unique_ptr<bittorrent::Peer>> try_peer(
    bittorrent::Torrent const& torrent) {
    std::error_code ec;
    auto get_tracker_info = torrent.discover_peers(ec);
    if (ec) {
        std::cerr << "Error discovering peers: " << ec << std::endl;
        return {};
    }
    bittorrent::TrackerInfo tracker_info = get_tracker_info.value();
    if (tracker_info.peers.size() == 0) {
        std::cerr << "No peers found" << std::endl;
        return {};
    }

    // get a peer to handshake
    std::optional<std::unique_ptr<bittorrent::Peer>> peer_to_handshake = {};
    for (std::string const& peer : tracker_info.peers) {
        std::string peer_ip = peer.substr(0, peer.find(':'));
        std::string peer_port_str = peer.substr(peer.find(':') + 1);

        peer_to_handshake = std::make_unique<bittorrent::Peer>(
            peer_ip, std::stoi(peer_port_str), torrent);
        bittorrent::Peer& p = *peer_to_handshake.value();

        ec = p.establish_connection();
        if (ec) {
            peer_to_handshake = {};
            continue;
        }

        ec = p.handshake(torrent.info_hash_raw());
        if (ec) {
            peer_to_handshake = {};
            continue;
        }

        break;
    }

    if (!peer_to_handshake.has_value()) {
        std::cerr << "Could not connect to any peers" << std::endl;
        return {};
    }

    return peer_to_handshake;
}

static int download_piece(std::string const& file_out,
                          std::string const& torrent_path, size_t piece_index) {
    std::error_code ec;
    auto get_torrent = bittorrent::Torrent::parse_torrent(torrent_path, ec);
    if (!get_torrent) {
        std::cerr << "Error parsing torrent: " << ec << std::endl;
        return 1;
    }
    bittorrent::Torrent const& torrent = *get_torrent;
    auto get_tracker_info = torrent.discover_peers(ec);
    if (ec) {
        std::cerr << "Error discovering peers: " << ec << std::endl;
        return 1;
    }
    bittorrent::TrackerInfo tracker_info = get_tracker_info.value();
    if (tracker_info.peers.size() == 0) {
        std::cerr << "No peers found" << std::endl;
        return 1;
    }

    // get a peer to handshake
    std::optional<std::unique_ptr<bittorrent::Peer>> peer_to_handshake =
        try_peer(torrent);
    if (!peer_to_handshake.has_value()) {
        std::cerr << "Could not connect to any peers" << std::endl;
        return 1;
    }

    bittorrent::Peer& p = *peer_to_handshake.value();

    ec = p.recv_bitfield();
    if (ec) {
        std::cerr << "Error receiving bitfield: " << ec << std::endl;
        return 1;
    }

    ec = p.interested_unchoke();
    if (ec) {
        std::cerr << "Error sending interested and unchoke: " << ec
                  << std::endl;
        return 1;
    }

    std::vector<uint8_t> piece = p.download_piece(piece_index, ec);
    if (ec) {
        std::cerr << "Error downloading piece: " << ec << std::endl;
        return 1;
    }

    std::ofstream file(file_out, std::ios::binary);
    file.write(reinterpret_cast<char const*>(piece.data()), piece.size());
    file.close();

    std::cout << "Downloaded piece " << piece_index << " to " << file_out
              << std::endl;

    return 0;
}

static int download_file(std::string const& file_out,
                         std::string const& torrent_path) {
    spdlog::debug("cli: downloading file");
    std::error_code ec;

    auto get_torrent = bittorrent::Torrent::parse_torrent(torrent_path, ec);
    if (!get_torrent) {
        std::cerr << "Error parsing torrent: " << ec << std::endl;
        return 1;
    }
    bittorrent::Torrent& torrent = *get_torrent;

    spdlog::debug("cli: torrent parsed");
    ec = torrent.download_file(file_out);
    if (ec) {
        std::cerr << "Error downloading file: " << ec << std::endl;
        return 1;
    }

    std::cout << "Downloaded " << torrent_path << " to " << file_out << "."
              << std::endl;

    return 0;
}

int main(int argc, char* argv[]) {
    // Enable debug logging:
    // spdlog::set_level(spdlog::level::debug);

    if (argc < 2) {
        std::cerr << "Usage: \t" << argv[0] << " decode <encoded_value>"
                  << std::endl;
        std::cerr << "\t " << argv[0] << " info <torrent file>" << std::endl;
        std::cerr << "\t " << argv[0] << " peers <torrent file>" << std::endl;
        std::cerr << "\t " << argv[0]
                  << " handshake <torrent file> <peer_ip>:<peer_port>"
                  << std::endl;
        std::cerr << "\t " << argv[0]
                  << " download_piece -o <output_file> <torrent file>"
                  << std::endl;
        std::cerr << "\t " << argv[0]
                  << " download -o <output_file> <torrent file>" << std::endl;
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

    else if (command == "download_piece") {
        if (argc < 6) {
            std::cerr << "Usage: " << argv[0]
                      << " download_piece -o <output_file> <torrent file> "
                         "<piece_index>"
                      << std::endl;
            return 1;
        }
        return download_piece(argv[3], argv[4], std::stoi(argv[5]));
    }

    else if (command == "download") {
        if (argc < 5) {
            std::cerr << "Usage: " << argv[0]
                      << " download -o <output_file> <torrent file>"
                      << std::endl;
            return 1;
        }
        return download_file(argv[3], argv[4]);
    }

    else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
