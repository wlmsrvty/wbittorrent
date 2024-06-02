#include "bencode.hpp"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <string>

using namespace bittorrent;
using json = nlohmann::json;

std::string bencode_string(std::string const& str) {
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

    val = Bencode::decode_bencoded_value("i42");
    CHECK_FALSE(val.has_value());

    val = Bencode::decode_bencoded_value("i42abce");
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
    CHECK(val.value() == json({"hello", {52}}));

    val = Bencode::decode_bencoded_value("llee");
    CHECK(val.has_value());
    CHECK(val.value() == json({json::array()}));

    val = Bencode::decode_bencoded_value("lli4eei5ee");
    CHECK(val.has_value());
    CHECK(val.value() == json({{4}, 5}));

    val = Bencode::decode_bencoded_value("l9:blueberryi609ee");
    CHECK(val.has_value());
    CHECK(val.value() == json({"blueberry", 609}));

    val = Bencode::decode_bencoded_value("li523e");
    CHECK_FALSE(val.has_value());

    val = Bencode::decode_bencoded_value("li42e");
    CHECK_FALSE(val.has_value());

    val = Bencode::decode_bencoded_value("lli42ei42ee");
    CHECK_FALSE(val.has_value());
}

TEST_CASE("Decoding dicts", "[bencode][decode]") {
    // d<key1><value1>...e
    auto val = Bencode::decode_bencoded_value("d3:foo3:bar5:helloi52ee");
    CHECK(val.has_value());
    CHECK(val.value() == json({{"foo", "bar"}, {"hello", 52}}));

    val = Bencode::decode_bencoded_value("d5:hellolee");
    CHECK(val.has_value());
    CHECK(val.value() == json({{"hello", json::array()}}));

    val = Bencode::decode_bencoded_value("d5:helloli42ei-5eee");
    CHECK(val.has_value());
    CHECK(val.value() == json({{"hello", {42, -5}}}));

    val = Bencode::decode_bencoded_value("d5:helloli42i-5eee");
    CHECK_FALSE(val.has_value());

    val = Bencode::decode_bencoded_value("d1:ae");
    CHECK_FALSE(val.has_value());

    val = Bencode::decode_bencoded_value("de");
    CHECK(val.has_value());
    CHECK(val.value() == json::object());

    val = Bencode::decode_bencoded_value(
        "d9:publisher3:bob17:publisher-webpage15:www.example.com18:publisher."
        "location4:homee");
    CHECK(val.has_value());
    CHECK(val.value() == json({{"publisher", "bob"},
                               {"publisher-webpage", "www.example.com"},
                               {"publisher.location", "home"}}));
}