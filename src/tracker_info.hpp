#pragma once

#include "lib/nlohmann/json.hpp"
#include <string>
#include <vector>

namespace bittorrent {

class TrackerInfo {
   public:
    TrackerInfo() = default;

    static TrackerInfo parse_tracker_response(nlohmann::json& tracker_response);

    // interval at which the client should wait between sending regular requests
    // to the tracker
    size_t interval;

    // list of peers
    // ipv4:port
    std::vector<std::string> peers;
};

}  // namespace bittorrent