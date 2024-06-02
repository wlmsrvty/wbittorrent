#include "bencode.hpp"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <string>

using namespace bittorrent;
using json = nlohmann::json;

TEST_CASE("Encoding strings", "[bencode][encode]") {
    json test = "hello";
    auto val = Bencode::encode_bencoded_value(test);
    CHECK(val == "5:hello");

}

TEST_CASE("Encoding integers", "[bencode][encode]") {
    json test = 42;
    auto val = Bencode::encode_bencoded_value(test);
    CHECK(val == "i42e");
}

TEST_CASE("Encoding lists", "[bencode][encode]") {
    json test = {1, 2, 3};
    auto val = Bencode::encode_bencoded_value(test);
    CHECK(val == "li1ei2ei3ee");
}

TEST_CASE("Encoding random", "[bencode][encode]") {
    json test = { { "1", "a"}, { "2", "b" }, };
    INFO(test.dump());
    auto val = Bencode::encode_bencoded_value(test);
    CHECK(val == "d1:11:a1:21:be");
}