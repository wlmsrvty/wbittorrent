#include "torrent.hpp"
#include "bencode.hpp"
#include "error.hpp"
#include "lib/sha1.hpp"
#include <fstream>
#include <iostream>

namespace bittorrent {
using errors::Error;
using json = nlohmann::json;

nonstd::expected<Torrent, errors::Error> Torrent::parse_torrent(
    // parsing metainfo torrent
    // https://www.bittorrent.org/beps/bep_0003.html#metainfo-files

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
    torrent.pieces = dict["info"]["pieces"];

    return torrent;
}

static std::vector<uint32_t> sha1_hash(std::string const& data) {
    sha1::SHA1 s;
    s.processBytes(data.c_str(), data.size());
    std::vector<uint32_t> digest(5);
    s.getDigest(digest.data());
    return digest;
}

std::string Torrent::info_hash() const {
    json info = {
        {"length", length},
        {"name", name},
        {"piece length", piece_length},
        {"pieces", pieces},
    };
    auto encoded_info = Bencode::encode_bencoded_value(info);
    auto digest = sha1_hash(encoded_info);
    char sha[48];
    snprintf(sha, 45, "%08x%08x%08x%08x%08x", digest[0], digest[1],
             digest[2], digest[3], digest[4]);
    return sha;
}

    std::vector<std::string> Torrent::piece_hashes() const {
        std::vector<std::string> result;
        auto it = pieces.begin();
        while (it != pieces.end()) {
            std::string piece_hash(it, it + 20);
            it += 20;
            result.push_back(piece_hash);
        }
        return result;
    }

}  // namespace bittorrent