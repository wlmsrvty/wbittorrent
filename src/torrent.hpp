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
    static nonstd::expected<Torrent, errors::Error> parse_torrent(
        std::filesystem::path const& file_path);
    
    std::string info_hash() const;

    // URL to tracker
    std::string announce;

    // INFO
    size_t length;
    std::string name;
    size_t piece_length;
    std::string pieces;
};
}  // namespace bittorrent