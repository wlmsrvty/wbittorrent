#include "lib/utils.hpp"
#include "lib/sha1.hpp"

namespace bittorrent {
namespace utils {

std::string url_encode(std::string const& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n;
         ++i) {
        std::string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char)c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

static std::vector<uint32_t> sha1_hash_internal(uint8_t const* data, size_t size) {
    sha1::SHA1 s;
    s.processBytes(data, size);
    std::vector<uint32_t> digest(5);
    s.getDigest(digest.data());
    return digest;
}

std::vector<uint8_t> hex_to_bytes(std::string const& hex) {
    std::vector<uint8_t> bytes;

    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        char byte = (char)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }

    return bytes;
}

std::vector<uint8_t> sha1_hash(uint8_t const* data, size_t size) {
    auto digest = sha1_hash_internal(data, size);
    std::vector<uint8_t> result;
    for (uint32_t d : digest) {
        result.push_back((d >> 24) & 0xFF);
        result.push_back((d >> 16) & 0xFF);
        result.push_back((d >> 8) & 0xFF);
        result.push_back(d & 0xFF);
    }
    return result;
}

}  // namespace utils

}  // namespace bittorrent