#include "tracker_info.hpp"

namespace bittorrent {

TrackerInfo TrackerInfo::parse_tracker_response(nlohmann::json& tracker_response) {
    TrackerInfo tracker_info;

    tracker_info.interval = tracker_response["interval"];

    auto peers = tracker_response["peers"];
    if (!peers.is_string()) {
        // TODO: error handling
        return tracker_info;
    }
    auto peers_str = peers.get<std::string>();

    size_t number_of_peers = peers_str.size() / 6;
    for (size_t i = 0; i < number_of_peers; i++) {
        std::string ip_raw = peers_str.substr(i * 6, 4);
        std::string ip = std::to_string(static_cast<unsigned char>(ip_raw[0])) + "." +
                         std::to_string(static_cast<unsigned char>(ip_raw[1])) + "." +
                         std::to_string(static_cast<unsigned char>(ip_raw[2])) + "." +
                         std::to_string(static_cast<unsigned char>(ip_raw[3]));
        std::string port_raw = peers_str.substr(i * 6 + 4, 2);
        uint16_t port =
            (static_cast<uint16_t>(static_cast<unsigned char>(port_raw[0])
                                   << 8)) |
            static_cast<uint16_t>(static_cast<unsigned char>(port_raw[1]));
        tracker_info.peers.push_back(ip + ":" + std::to_string(port));
    }

    return tracker_info;
}
}  // namespace bittorrent

