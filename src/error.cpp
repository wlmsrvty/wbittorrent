#include "error.hpp"
#include <system_error>

namespace bittorrent {
namespace errors {

std::error_code make_error_code(error_code_enum e) {
    // TODO: new category for bittorrent errors
    return std::error_code(static_cast<int>(e), std::system_category());
}

}  // namespace errors
}  // namespace bittorrent

// Register the enum as an error code enum
template <>
struct std::is_error_code_enum<bittorrent::errors::error_code_enum>
    : std::true_type {};