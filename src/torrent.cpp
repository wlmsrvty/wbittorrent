#include "torrent.hpp"
#include "bencode.hpp"
#include "error.hpp"
#include "httplib.h"
#include "lib/sha1.hpp"
#include "lib/utils.hpp"
#include "peer.hpp"
#include "spdlog/spdlog.h"
#include "tracker_info.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <string>

namespace bittorrent {

std::unique_ptr<Torrent> Torrent::parse_torrent(
    std::filesystem::path const& file_path, std::error_code& ec) {
    // parsing metainfo torrent
    // https://www.bittorrent.org/beps/bep_0003.html#metainfo-files
    // read file
    std::ifstream f(file_path);
    if (!f.good()) {
        ec = errors::make_error_code(errors::error_code_enum::parse_torrent);
        return {};
    }
    std::stringstream buffer;
    buffer << f.rdbuf();

    // parse bencoded dict
    auto encoded_value = Bencode::decode_bencoded_value(buffer.str(), ec);
    if (ec) return {};
    auto dict = encoded_value;

    std::unique_ptr<Torrent> torrent = std::make_unique<Torrent>();
    torrent->announce = dict["announce"];
    torrent->length = dict["info"]["length"];
    torrent->name = dict["info"]["name"];
    torrent->piece_length = dict["info"]["piece length"];
    torrent->pieces = dict["info"]["pieces"];

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

std::optional<TrackerInfo> Torrent::discover_peers(std::error_code& ec) const {
    spdlog::debug("Discovering peers from tracker: {}", announce);

    static std::regex const re(
        R"(^((?:(https?):)?(?://([^:/?#]*)(?::(\d+))?)?)(([^?#]*(?:\?[^#]*)?)(?:#.*)?))");
    std::smatch matches;
    if (!std::regex_match(announce, matches, re)) {
        ec = errors::make_error_code(
            errors::error_code_enum::discover_peers_invalid_announce_url);
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

    static std::string_view const peer_id = "00112233445566778899";
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
        ec = errors::make_error_code(
            errors::error_code_enum::discover_peers_http);
        return {};
    }

    spdlog::debug("Torrent: Tracker response: {}", res->body);

    auto tracker_response = Bencode::decode_bencoded_value(res->body, ec);
    if (ec) return {};

    TrackerInfo tracker_info =
        TrackerInfo::parse_tracker_response(tracker_response);
    
    spdlog::debug("Torrent: Number of peers: {}", tracker_info.peers.size());

    return tracker_info;
}

void Torrent::connect_peers() {
    spdlog::debug("Torrent: Connecting to peers");
    std::error_code ec;

    std::optional<TrackerInfo> tracker_info = discover_peers(ec);
    if (tracker_info.has_value() == false) return;

    TrackerInfo info = tracker_info.value();

    std::vector<uint8_t> digest = info_hash_raw();

    for (std::string const& peer : info.peers) {
        std::string peer_ip = peer.substr(0, peer.find(':'));
        std::string peer_port_str = peer.substr(peer.find(':') + 1);

        std::unique_ptr<Peer> p = std::make_unique<bittorrent::Peer>(
            peer_ip, std::stoi(peer_port_str), *this);

        ec = p->establish_connection();
        if (ec) continue;

        ec = p->handshake(digest);
        if (ec) continue;

        ec = p->recv_bitfield();
        if (ec) continue;

        ec = p->interested_unchoke();
        if (ec) continue;

        spdlog::debug("Torrent: Peer connected: {}", peer);
        peers.push_back(std::move(p));
    }

    return;
}

std::vector<Torrent::piece> Torrent::compute_pieces_to_download(
    std::string const& out_file_path) {
    std::vector<piece> result;
    std::vector<std::string> hashes = piece_hashes();

    // file does not exist, all pieces must be downloaded
    if (!std::filesystem::exists(out_file_path)) {
        for (size_t i = 0; i < hashes.size(); i++)
            result.push_back(
                {i, std::vector<uint8_t>(hashes[i].begin(), hashes[i].end())});
        return result;
    }

    // TODO:
    // file exists, check pieces that exists in the file
    // std::ifstream f(out_file_path, std::ios::binary);
    // size_t piece_index = 0;
    // std::vector<uint8_t> buffer;
    // buffer.reserve(piece_length);

    // for now, request to ALWAYS download all pieces
    for (size_t i = 0; i < hashes.size(); i++) {
        result.push_back(
            {i, std::vector<uint8_t>(hashes[i].begin(), hashes[i].end())});
    }

    return result;
}

std::error_code Torrent::download_file(std::string const& out_file_path) {
    if (peers.size() == 0) {
        spdlog::debug("Torrent: searching peers");
        connect_peers();
        spdlog::debug("Torrent: searching peers done, number of peers: {}",
                      peers.size());
        if (peers.size() == 0) {
            std::error_code ec = errors::make_error_code(
                errors::error_code_enum::torrent_download_file_no_peers);
            return ec;
        }
    }

    std::vector<Torrent::piece> pieces_to_download =
        compute_pieces_to_download(out_file_path);

    std::error_code ec;

    Peer& p = *peers[0];
    std::ofstream f(out_file_path, std::ios::binary);
    for (size_t i = 0; i < pieces_to_download.size(); i++) {
        std::vector<uint8_t> piece_data =
            p.download_piece(pieces_to_download[i].first, ec);
        if (ec) return ec;
        f.write(reinterpret_cast<char*>(piece_data.data()), piece_data.size());
    }

    return {};
}

}  // namespace bittorrent