#pragma once

#include "error.hpp"
#include "lib/nlohmann/json.hpp"
#include "lib/nonstd/expected.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace bittorrent {
class Torrent {
   public:
    Torrent() = default;

    // Parse a torrent file
    static nonstd::expected<Torrent, errors::Error> parse_torrent(
        std::filesystem::path const& file_path);

    // Returns SHA1 of the info dictionary
    std::vector<uint8_t> info_hash_raw() const;
    std::string info_hash() const;

    // Returns a vector of 20-byte SHA1 hashes
    std::vector<std::string> piece_hashes() const;

    // Request tracker for peers
    nlohmann::json discover_peers() const;

    // URL to tracker
    std::string announce;

    // INFO
    size_t length;
    std::string name;
    size_t piece_length;
    std::string pieces;
};
}  // namespace bittorrent