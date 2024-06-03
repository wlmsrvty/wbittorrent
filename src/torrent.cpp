#include "torrent.hpp"
#include "bencode.hpp"
#include "error.hpp"
#include "httplib.h"
#include "lib/sha1.hpp"
#include "lib/utils.hpp"
#include "tracker_info.hpp"

#include <fstream>
#include <iostream>
#include <regex>
#include <string>

namespace bittorrent {
using errors::Error;

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

static std::vector<uint8_t> hex_to_bytes(std::string const& hex) {
    std::vector<uint8_t> bytes;

    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        char byte = (char)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }

    return bytes;
}

std::vector<uint8_t> Torrent::info_hash_raw() const {
    std::string digest_str = info_hash();
    return hex_to_bytes(digest_str);
}

std::string Torrent::info_hash() const {
    nlohmann::json info = {
        {"length", length},
        {"name", name},
        {"piece length", piece_length},
        {"pieces", pieces},
    };
    auto encoded_info = Bencode::encode_bencoded_value(info);
    auto digest = sha1_hash(encoded_info);
    char sha[48];
    snprintf(sha, 45, "%08x%08x%08x%08x%08x", digest[0], digest[1], digest[2],
             digest[3], digest[4]);
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

nlohmann::json Torrent::discover_peers() const {
    static const std::regex re(
        R"(^((?:(https?):)?(?://([^:/?#]*)(?::(\d+))?)?)(([^?#]*(?:\?[^#]*)?)(?:#.*)?))");
    std::smatch matches;
    if (!std::regex_match(announce, matches, re)) {
        return {};
    }
    std::string base_url = matches[1].str();
    std::string rest_url = matches[6].str();
    std::string tracker_url = rest_url;

    std::vector<uint8_t> digest = info_hash_raw();
    auto digest_str = std::string(reinterpret_cast<char*>(digest.data()),
                                  digest.size() * sizeof(uint8_t));
    std::string digest_2 = utils::url_encode(digest_str);
    tracker_url += "?info_hash=" + digest_2;

    const static std::string_view peer_id = "00112233445566778899";
    tracker_url += "&peer_id=" + std::string(peer_id);

    // port the client is listening on
    std::string port = "6881";
    tracker_url += "&port=" + port;

    // total amount uploaded
    size_t uploaded = 0;
    tracker_url += "&uploaded=" + std::to_string(uploaded);

    // total amount downloaded
    size_t downloaded = 0;
    tracker_url += "&downloaded=" + std::to_string(downloaded);

    // total amount left
    size_t left = length;
    tracker_url += "&left=" + std::to_string(left);

    // compact option
    int compact = 1;
    tracker_url += "&compact=" + std::to_string(compact);

    httplib::Client cli(base_url);
    auto res = cli.Get(tracker_url);
    if (!res) {
        return {};
    }

    auto tracker_response = Bencode::decode_bencoded_value(res->body);
    if (tracker_response.has_value() == false) {
        return {};
    }

    nlohmann::json val = tracker_response.value();
    TrackerInfo tracker_info = TrackerInfo::parse_tracker_response(val);
    for (const std::string& peer : tracker_info.peers) {
        std::cout << peer << std::endl;
    }

    return tracker_response.value();
}

}  // namespace bittorrent