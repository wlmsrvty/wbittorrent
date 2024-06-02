#include "lib/nlohmann/json.hpp"
#include "lib/nonstd/expected.hpp"
#include "error.hpp"
#include "bencode.hpp"
#include "torrent.hpp"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

using json = nlohmann::json;
using Error = bittorrent::errors::Error;

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
        auto get_torrent = bittorrent::Torrent::parse_torrent(argv[2]);
        if (get_torrent.has_value() == false) {
            std::cerr << "Error parsing torrent: " << get_torrent.error().message
                      << std::endl;
            return 1;
        }
        auto torrent = get_torrent.value();
        std::cout << "Tracker URL: " << torrent.announce << std::endl;
        std::cout << "Length: " << torrent.length << std::endl;
    }


     else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
