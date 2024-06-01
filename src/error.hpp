#pragma once

#include <string>

namespace bittorrent {
namespace errors {
enum class error_code {
    // Not an error
    no_error = 0,

    // Error decoding bencoded value
    decode,
    decode_parse,
    decode_not_handled
};

struct Error {
    Error(error_code code, std::string message)
        : code(code), message(message) {}
    std::string message;
    error_code code;
};
}  // namespace errors
}  // namespace bittorrent