#pragma once

#include <string>
#include <system_error>

namespace bittorrent {
namespace errors {

enum class error_code_enum {
    // Not an error
    ok = 0,

    // ## Bencode errors ##
    // error parsing string
    bencode_decode_parse_string,
    // error parsing integer
    bencode_decode_parse_integer,
    // error parsing list
    bencode_decode_parse_list,
    // error parsing dict
    bencode_decode_parse_dict,
    // key in dict is not a string
    bencode_decode_parse_dict_key,
    // error parsing eof
    bencode_decode_parse_eof,
    // invalid bencode value
    bencode_decode_invalid,

    discover_peers_http,
    discover_peers_invalid_announce_url,

    parse_torrent
};

std::error_code make_error_code(error_code_enum e);

}  // namespace errors
}  // namespace bittorrent
