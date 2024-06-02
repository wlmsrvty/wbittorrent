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