#include "bencode.hpp"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <string>
#include <system_error>

using namespace bittorrent;
using json = nlohmann::json;

std::string bencode_string(std::string const& str) {
    return std::to_string(str.size()) + ":" + str;
}
static std::error_code ec = {};

TEST_CASE("Decoding strings", "[bencode][decode]") {
    ec = {};
    auto expected = "hello";
    auto val = Bencode::decode_bencoded_value(bencode_string(expected), ec);
    CHECK_FALSE(ec);
    CHECK(val == json(expected));

    ec = {};
    val = Bencode::decode_bencoded_value("1:a", ec);
    CHECK_FALSE(ec);
    CHECK(val == json("a"));

    ec = {};
    val = Bencode::decode_bencoded_value("0:", ec);
    CHECK_FALSE(ec);
    CHECK(val == json(""));
}

TEST_CASE("Decoding integers", "[bencode][decode]") {
    ec = {};
    auto val = Bencode::decode_bencoded_value("i3e", ec);
    CHECK_FALSE(ec);
    CHECK(val == json(3));

    ec = {};
    val = Bencode::decode_bencoded_value("i1251910465e", ec);
    CHECK_FALSE(ec);
    CHECK(val == json(1251910465));

    ec = {};
    val = Bencode::decode_bencoded_value("i-52e", ec);
    CHECK_FALSE(ec);
    CHECK(val == json(-52));

    ec = {};
    val = Bencode::decode_bencoded_value("i4294967300e", ec);
    CHECK_FALSE(ec);
    CHECK(val == json(4294967300));

    ec = {};
    val = Bencode::decode_bencoded_value("i0e", ec);
    CHECK_FALSE(ec);
    CHECK(val == json(0));

    ec = {};
    val = Bencode::decode_bencoded_value("i-0e", ec);
    CHECK(ec);

    ec = {};
    val = Bencode::decode_bencoded_value("i42", ec);
    CHECK(ec);

    ec = {};
    val = Bencode::decode_bencoded_value("i42abce", ec);
    CHECK(ec);
}

TEST_CASE("Decoding lists", "[bencode][decode]") {
    // l<bencoded_elements>e
    ec = {};
    auto val = Bencode::decode_bencoded_value("l5:helloi52ee", ec);
    CHECK_FALSE(ec);
    CHECK(val == json({"hello", 52}));

    ec = {};
    val = Bencode::decode_bencoded_value("le", ec);
    CHECK_FALSE(ec);
    CHECK(val == json::array());

    ec = {};
    val = Bencode::decode_bencoded_value("l5:helloli52eee", ec);
    CHECK_FALSE(ec);
    CHECK(val == json({"hello", {52}}));

    ec = {};
    val = Bencode::decode_bencoded_value("llee", ec);
    CHECK_FALSE(ec);
    CHECK(val == json::array({json::array()}));

    ec = {};
    val = Bencode::decode_bencoded_value("lli4eei5ee", ec);
    CHECK_FALSE(ec);
    CHECK(val == json({{4}, 5}));

    ec = {};
    val = Bencode::decode_bencoded_value("l9:blueberryi609ee", ec);
    CHECK_FALSE(ec);
    CHECK(val == json({"blueberry", 609}));

    ec = {};
    val = Bencode::decode_bencoded_value("li523e", ec);
    CHECK(ec);

    ec = {};
    val = Bencode::decode_bencoded_value("li42e", ec);
    CHECK(ec);

    ec = {};
    val = Bencode::decode_bencoded_value("lli42ei42ee", ec);
    CHECK(ec);
}

TEST_CASE("Decoding dicts", "[bencode][decode]") {
    // d<key1><value1>...e
    ec = {};
    auto val = Bencode::decode_bencoded_value("d3:foo3:bar5:helloi52ee", ec);
    CHECK_FALSE(ec);
    CHECK(val == json({{"foo", "bar"}, {"hello", 52}}));

    ec = {};
    val = Bencode::decode_bencoded_value("d5:hellolee", ec);
    CHECK_FALSE(ec);
    CHECK(val == json({{"hello", json::array()}}));

    ec = {};
    val = Bencode::decode_bencoded_value("d5:helloli42ei-5eee", ec);
    CHECK_FALSE(ec);
    CHECK(val == json({{"hello", {42, -5}}}));

    ec = {};
    val = Bencode::decode_bencoded_value("d5:helloli42i-5eee", ec);
    CHECK(ec);

    ec = {};
    val = Bencode::decode_bencoded_value("d1:ae", ec);
    CHECK(ec);

    ec = {};
    val = Bencode::decode_bencoded_value("de", ec);
    CHECK_FALSE(ec);
    CHECK(val == json::object());

    ec = {};
    val = Bencode::decode_bencoded_value(
        "d9:publisher3:bob17:publisher-webpage15:www.example.com18:publisher."
        "location4:homee",
        ec);
    CHECK_FALSE(ec);
    CHECK(val == json({{"publisher", "bob"},
                       {"publisher-webpage", "www.example.com"},
                       {"publisher.location", "home"}}));
}