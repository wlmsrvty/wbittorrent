#include "bencode.hpp"
#include "error.hpp"

using json = nlohmann::json;
using Error = bittorrent::errors::Error;
using nonstd::expected;

namespace bittorrent {
static expected<json, Error> decode(std::string const& encoded_value,
                                    std::string::const_iterator& it) {
    // Decode a bencoded value
    // Spec: https://wiki.theory.org/BitTorrentSpecification#Bencoding
    if (it == encoded_value.end()) {
        return nonstd::make_unexpected(
            Error(errors::error_code::decode_eof, "Unexpected end of input"));
    }

    // strings: <size>:<content>
    // 4:spam -> "spam"
    if (std::isdigit(*it)) {
        auto colon_index = std::find(it, std::end(encoded_value), ':');
        if (colon_index == std::end(encoded_value))
            return nonstd::make_unexpected(
                Error(errors::error_code::decode_parse,
                      "Invalid encoded value: " + encoded_value));
        std::string size_string = std::string(it, colon_index);
        int64_t number = std::atoll(size_string.c_str());
        std::string str =
            std::string(colon_index + 1, colon_index + 1 + number);
        it = colon_index + number + 1;
        return json(str);
    }

    // integers: i<number>e
    // i3e -> 3
    else if (*it == 'i') {
        it++;
        auto e_index = std::find(it, std::end(encoded_value), 'e');
        if (e_index == std::end(encoded_value))
            return nonstd::make_unexpected(
                Error(errors::error_code::decode_parse,
                      "Invalid encoded value: " + encoded_value));
        // Check for leading zeroes: i03e
        if (*it == '0' && std::distance(it, e_index) > 1)
            return nonstd::make_unexpected(
                Error(errors::error_code::decode_parse,
                      "Invalid encoded value: " + encoded_value));
        // Check for negative leading zeroes: i-0e
        if (*it == '-' && (it + 1) != std::end(encoded_value) &&
            *(it + 1) == '0')
            return nonstd::make_unexpected(
                Error(errors::error_code::decode_parse,
                      "Invalid encoded value: " + encoded_value));
        auto number_string = std::string(it, e_index);
        int64_t number = std::atoll(number_string.c_str());
        it = e_index + 1;
        return json(number);
    }

    // lists: l<bencoded_elements>e
    // l5:helloi52ee -> ["hello", 52]
    else if (*it == 'l') {
        it++;
        auto list = json::array();
        while (it != std::end(encoded_value) && *it != 'e') {
            auto val = decode(encoded_value, it);
            if (!val.has_value()) return val;
            list.push_back(val.value());
        }
        if (it == std::end(encoded_value) || *it != 'e')
            return nonstd::make_unexpected(
                Error(errors::error_code::decode_parse,
                      "Invalid encoded value: " + encoded_value));
        it++;
        return list;
    }

    else {
        return nonstd::make_unexpected(
            Error(errors::error_code::decode,
                  "Unhandled encoded value: " + encoded_value));
    }
}

nonstd::expected<nlohmann::json, errors::Error> Bencode::decode_bencoded_value(
    std::string const& encoded_value) {
    // Decode a bencoded value
    // Spec: https://wiki.theory.org/BitTorrentSpecification#Bencoding
    auto it = encoded_value.begin();
    return decode(encoded_value, it);
}
}  // namespace bittorrent