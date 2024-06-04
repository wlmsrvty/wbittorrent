#pragma once

#include "error.hpp"
#include "lib/nlohmann/json.hpp"
#include "lib/nonstd/expected.hpp"
#include "tracker_info.hpp"
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

namespace bittorrent {
class Torrent {
   public:
    Torrent() = default;

    // Parse a torrent file
    static std::optional<Torrent> parse_torrent(
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

    // INFO
    size_t length;
    std::string name;
    size_t piece_length;
    std::string pieces;
};
}  // namespace bittorrent