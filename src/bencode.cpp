#include "bencode.hpp"
#include "error.hpp"

using json = nlohmann::json;

namespace bittorrent {

static nlohmann::json decode(std::string const& encoded_value,
                             std::string::const_iterator& it,
                             std::error_code& ec) {
    if (it == encoded_value.end()) {
        ec = errors::make_error_code(
            errors::error_code_enum::bencode_decode_parse_eof);
        return {};
    }

    // strings: <size>:<content>
    // 4:spam -> "spam"
    if (std::isdigit(*it)) {
        auto colon_index = std::find(it, std::end(encoded_value), ':');
        if (colon_index == std::end(encoded_value)) {
            ec = errors::make_error_code(
                errors::error_code_enum::bencode_decode_parse_string);
            return {};
        }
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
        if (e_index == std::end(encoded_value)) {
            ec = errors::make_error_code(
                errors::error_code_enum::bencode_decode_parse_integer);
            return {};
        }

        // Check for leading zeroes: i03e
        if (*it == '0' && std::distance(it, e_index) > 1) {
            ec = errors::make_error_code(
                errors::error_code_enum::bencode_decode_parse_integer);
            return {};
        }

        // Check for negative leading zeroes: i-0e
        if (*it == '-' && (it + 1) != std::end(encoded_value) &&
            *(it + 1) == '0') {
            ec = errors::make_error_code(
                errors::error_code_enum::bencode_decode_parse_integer);
            return {};
        }

        // Check all digits
        auto begin = (*it == '-') ? it + 1 : it;
        auto digits = std::all_of(begin, e_index, isdigit);
        if (!digits) {
            ec = errors::make_error_code(
                errors::error_code_enum::bencode_decode_parse_integer);
            return {};
        }

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
            auto val = decode(encoded_value, it, ec);
            if (ec) return {};
            list.push_back(val);
        }
        if (it == std::end(encoded_value) || *it != 'e') {
            ec = errors::make_error_code(
                errors::error_code_enum::bencode_decode_parse_list);
            return {};
        }
        it++;
        return list;
    }

    // dicts: d<key1><value1>...e
    else if (*it == 'd') {
        it++;
        auto dict = json::object();
        while (it != std::end(encoded_value) && *it != 'e') {
            auto key = decode(encoded_value, it, ec);
            if (ec) return {};
            if (key.is_string() == false)  // key must be a string
            {
                ec = errors::make_error_code(
                    errors::error_code_enum::bencode_decode_parse_dict_key);
                return {};
            }
            auto val = decode(encoded_value, it, ec);
            if (ec) return {};
            dict[key] = val;
        }
        if (it == std::end(encoded_value) || *it != 'e') {
            ec = errors::make_error_code(
                errors::error_code_enum::bencode_decode_parse_dict);
            return {};
        }
        it++;
        return dict;
    }

    else {
        ec = errors::make_error_code(
            errors::error_code_enum::bencode_decode_invalid);
        return {};
    }
}

nlohmann::json Bencode::decode_bencoded_value(std::string const& encoded_value,
                                              std::error_code& ec) {
    // Decode a bencoded value
    // Spec: https://wiki.theory.org/BitTorrentSpecification#Bencoding
    auto it = encoded_value.begin();
    return decode(encoded_value, it, ec);
}

std::string encode(nlohmann::json const& value) {
    std::string result;
    if (value.is_string()) {
        result = std::to_string(value.get<std::string>().size()) + ":" +
                 value.get<std::string>();
    } else if (value.is_number_integer()) {
        result = "i" + std::to_string(value.get<int64_t>()) + "e";
    } else if (value.is_array()) {
        result = "l";
        for (auto const& v : value) {
            result += encode(v);
        }
        result += "e";
    } else if (value.is_object()) {
        result = "d";
        for (auto const& [key, val] : value.items()) {
            result += encode(key);
            result += encode(val);
        }
        result += "e";
    }
    return result;
}

std::string Bencode::encode_bencoded_value(nlohmann::json const& value) {
    // Encode a value into bencoded format
    // Spec: https://wiki.theory.org/BitTorrentSpecification#Bencoding
    return encode(value);
}

}  // namespace bittorrent