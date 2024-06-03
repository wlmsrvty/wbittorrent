#include "bencode.hpp"
#include "error.hpp"
#include "lib/nlohmann/json.hpp"
#include "lib/nonstd/expected.hpp"
#include "torrent.hpp"
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

using json = nlohmann::json;
using Error = bittorrent::errors::Error;

static int torrent_info(std::string const& file_path) {
    auto get_torrent = bittorrent::Torrent::parse_torrent(file_path);
    if (get_torrent.has_value() == false) {
        std::cerr << "Error parsing torrent: " << get_torrent.error().message
                  << std::endl;
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

static int discover_peers(const std::string& file_path) {
    auto get_torrent = bittorrent::Torrent::parse_torrent(file_path);
    if (get_torrent.has_value() == false) {
        std::cerr << "Error parsing torrent: " << get_torrent.error().message
                  << std::endl;
        return 1;
    }
    auto torrent = get_torrent.value();
    auto tracker_info = torrent.discover_peers();
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
        nonstd::expected<json, Error> decoded_value =
            bittorrent::Bencode::decode_bencoded_value(encoded_value);

        if (!decoded_value) {
            std::cerr << "Error decoding bencoded value: "
                      << decoded_value.error().message << std::endl;
            std::abort();
        }
        std::cout << decoded_value.value().dump() << std::endl;
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

    else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
