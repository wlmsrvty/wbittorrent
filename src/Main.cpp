#include "lib/nlohmann/json.hpp"
#include <cctype>
#include <cstdlib>
#include <expected>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

using json = nlohmann::json;

namespace bittorrent {
namespace errors {
enum class error_code {
    // Not an error
    no_error = 0,

    // Error decoding bencoded value
    decode,
    decode_parse,
    decode_not_handled
};

struct Error {
    Error(error_code code, std::string message)
        : code(code), message(std::move(message)) {}
    std::string message;
    error_code code;
};
}  // namespace errors
}  // namespace bittorrent

using Error = bittorrent::errors::Error;

//  Decode a bencoded value
//  Spec: https://wiki.theory.org/BitTorrentSpecification#Bencoding
std::expected<json, Error> decode_bencoded_value(
    std::string const& encoded_value) {
    // strings: <size>:<content>
    if (std::isdigit(encoded_value[0])) {
        size_t colon_index = encoded_value.find(':');
        if (colon_index != std::string::npos) {
            std::string number_string = encoded_value.substr(0, colon_index);
            int64_t number = std::atoll(number_string.c_str());
            std::string str = encoded_value.substr(colon_index + 1, number);
            return json(str);
        } else {
            return std::unexpected(
                Error(bittorrent::errors::error_code::decode,
                      "Invalid encoded value: " + encoded_value));
        }
    }
    // integers: i<number>e
    else if (encoded_value[0] == 'i') {
        size_t e_index = encoded_value.find('e');
        if (e_index != std::string::npos) {
            std::string number_string = encoded_value.substr(1, e_index - 1);
            int64_t number = std::atoll(number_string.c_str());
            return json(number);
        } else {
            return std::unexpected(
                Error(bittorrent::errors::error_code::decode,
                      "Invalid encoded value: " + encoded_value));
        }
    } else {
        return std::unexpected(
            Error(bittorrent::errors::error_code::decode,
                  "Unhandled encoded value: " + encoded_value));
    }
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
        std::expected<json, Error> decoded_value =
            decode_bencoded_value(encoded_value);
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
