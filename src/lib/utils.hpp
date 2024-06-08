#pragma once

#include "sha1.h"
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace bittorrent {
namespace utils {

std::string url_encode(std::string const& value);

std::vector<uint8_t> hex_to_bytes(std::string const& hex);
std::vector<uint8_t> sha1_hash(uint8_t const* data, size_t size);

}  // namespace utils
}  // namespace bittorrent