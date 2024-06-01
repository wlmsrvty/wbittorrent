#include "bencode.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace bittorrent;
using json = nlohmann::json;

std::string bencode_string(const std::string& str) {
    return std::to_string(str.size()) + ":" + str;
}

TEST_CASE("Decoding strings", "[bencode][decode]") {
    auto expected = "hello";
    auto val = Bencode::decode_bencoded_value(bencode_string(expected));
    REQUIRE(val.has_value());
    REQUIRE(val.value() == json(expected));

    val = Bencode::decode_bencoded_value("1:a");
    REQUIRE(val.has_value());
    REQUIRE(val.value() == json("a"));
}

TEST_CASE("Decoding integers", "[bencode][decode]") {
    auto val = Bencode::decode_bencoded_value("i3e");
    REQUIRE(val.has_value());
    REQUIRE(val.value() == json(3));
}