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
    CHECK(val.has_value());
    CHECK(val.value() == json(expected));

    val = Bencode::decode_bencoded_value("1:a");
    CHECK(val.has_value());
    CHECK(val.value() == json("a"));

    val = Bencode::decode_bencoded_value("0:");
    CHECK(val.has_value());
    CHECK(val.value() == json(""));
}

TEST_CASE("Decoding integers", "[bencode][decode]") {
    auto val = Bencode::decode_bencoded_value("i3e");
    CHECK(val.has_value());
    CHECK(val.value() == json(3));

    val = Bencode::decode_bencoded_value("i1251910465e");
    CHECK(val.has_value());
    CHECK(val.value() == json(1251910465));

    val = Bencode::decode_bencoded_value("i-52e");
    CHECK(val.has_value());
    CHECK(val.value() == json(-52));

    val = Bencode::decode_bencoded_value("i4294967300e");
    CHECK(val.has_value());
    CHECK(val.value() == json(4294967300));

    val = Bencode::decode_bencoded_value("i0e");
    CHECK(val.has_value());
    CHECK(val.value() == json(0));

    val = Bencode::decode_bencoded_value("i-0e");
    CHECK_FALSE(val.has_value());
}

TEST_CASE("Decoding lists", "[bencode][decode]") {
    // l<bencoded_elements>e
    auto val = Bencode::decode_bencoded_value("l5:helloi52ee");
    CHECK(val.has_value());
    CHECK(val.value() == json({"hello", 52}));

    val = Bencode::decode_bencoded_value("le");
    CHECK(val.has_value());
    CHECK(val.value() == json::array());

    val = Bencode::decode_bencoded_value("l5:helloli52eee");
    CHECK(val.has_value());
    CHECK(val.value() == json({"hello", { 52 } }));

    val = Bencode::decode_bencoded_value("llee");
    CHECK(val.has_value());
    CHECK(val.value() == json({ json::array() }));
}