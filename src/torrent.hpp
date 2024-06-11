#pragma once

#include "error.hpp"
#include "lib/nlohmann/json.hpp"
#include "lib/nonstd/expected.hpp"
#include "tracker_info.hpp"
#include "peer.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace bittorrent {

class Torrent {
   public:
    Torrent() = default;
    ~Torrent() = default;

    // Delete copy constructor and copy assignment operator
    Torrent(const Torrent&) = delete;
    Torrent& operator=(const Torrent&) = delete;

    // Parse a torrent file
    static std::unique_ptr<Torrent> parse_torrent(
        std::filesystem::path const& file_path, std::error_code& ec);

    // Returns SHA1 of the info dictionary
    std::vector<uint8_t> info_hash_raw() const;
    std::string info_hash() const;

    // Returns a vector of 20-byte SHA1 hashes
    std::vector<std::string> piece_hashes() const;

    // Request tracker for peers
    std::optional<TrackerInfo> discover_peers(std::error_code& ec) const;

    // URL to tracker
    std::string announce;

    void connect_peers();

    using piece_hash = std::vector<uint8_t>;
    using piece_index = size_t;
    using piece = std::pair<piece_index, piece_hash>;
    std::vector<piece> compute_pieces_to_download(
        std::string const& out_file_path);

    std::error_code download_file(std::string const& out_file_path);

    // INFO
    size_t length;
    std::string name;
    size_t piece_length;
    std::string pieces;

    std::vector<std::unique_ptr<Peer>> peers;
};

}  // namespace bittorrent