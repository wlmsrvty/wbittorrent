#include "bencode.hpp"
#include "error.hpp"

using json = nlohmann::json;
using Error = bittorrent::errors::Error;

namespace bittorrent {

nonstd::expected<nlohmann::json, errors::Error> Bencode::decode_bencoded_value(
    std::string const& encoded_value) {
    // Decode a bencoded value
    // Spec: https://wiki.theory.org/BitTorrentSpecification#Bencoding
    // strings: <size>:<content>
    // 4:spam -> "spam"
    if (std::isdigit(encoded_value[0])) {
        size_t colon_index = encoded_value.find(':');
        if (colon_index != std::string::npos) {
            std::string number_string = encoded_value.substr(0, colon_index);
            int64_t number = std::atoll(number_string.c_str());
            std::string str = encoded_value.substr(colon_index + 1, number);
            return json(str);
        } else {
            return nonstd::unexpected(
                Error(errors::error_code::decode,
                      "Invalid encoded value: " + encoded_value));
        }
    }
    // integers: i<number>e
    // i3e -> 3
    else if (encoded_value[0] == 'i') {
        size_t e_index = encoded_value.find('e');
        if (e_index != std::string::npos) {
            std::string number_string = encoded_value.substr(1, e_index - 1);
            int64_t number = std::atoll(number_string.c_str());
            return json(number);
        } else {
            return nonstd::make_unexpected(
                Error(errors::error_code::decode,
                      "Invalid encoded value: " + encoded_value));
        }
    } else {
        return nonstd::make_unexpected(
            Error(errors::error_code::decode,
                  "Unhandled encoded value: " + encoded_value));
    }
}
}  // namespace bittorrent