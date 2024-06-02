#include "torrent.hpp"
#include "bencode.hpp"
#include "error.hpp"
#include <fstream>
#include <iostream>

namespace bittorrent {
using errors::Error;
nonstd::expected<Torrent, errors::Error> Torrent::parse_torrent(
    std::filesystem::path const& file_path) {
    // read file
    std::ifstream f(file_path);
    if (!f.good())
        return nonstd::make_unexpected(
            Error(errors::error_code::parse_torrent, "Error reading file"));
    std::stringstream buffer;
    buffer << f.rdbuf();

    // parse bencoded dict
    auto encoded_value = Bencode::decode_bencoded_value(buffer.str());
    if (!encoded_value.has_value())
        return nonstd::make_unexpected(encoded_value.error());
    auto dict = encoded_value.value();

    Torrent torrent;
    torrent.announce = dict["announce"];
    torrent.length = dict["info"]["length"];
    torrent.name = dict["info"]["name"];
    torrent.piece_length = dict["info"]["piece length"];

    return torrent;
}
}  // namespace bittorrent