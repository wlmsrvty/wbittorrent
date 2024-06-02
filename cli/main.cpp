#include "lib/nlohmann/json.hpp"
#include "lib/nonstd/expected.hpp"
#include "error.hpp"
#include "bencode.hpp"

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
    } else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
