#include <catch.hpp>
#include <optional>
#include <sstream>
#include <variant>
#include "io/io.hpp"

TEST_CASE("can write fundamental types", "[io]") {
  std::ostringstream os{};
  int i = 0x12345678;
  io::writeBytes(os, i);
  REQUIRE(os.str() == "\x78\x56\x34\x12");
}

TEST_CASE("can write strings", "[io]") {
  std::ostringstream os{};
  std::string s = "hello, world";
  io::writeBytes(os, s);
  REQUIRE(os.str() == "hello, world");
}

TEST_CASE("can write pairs", "[io]") {
  std::ostringstream os{};
  int i = 0x12345678;
  std::string s = "hello, world";
  auto p = std::make_pair(i, s);
  io::writeBytes(os, p);
  REQUIRE(os.str() == "\x78\x56\x34\x12hello, world");
}

TEST_CASE("can write optionals", "[io]") {
  std::ostringstream os{};
  std::optional<int> opt = 0x12345678;
  io::writeBytes(os, opt);
  REQUIRE(os.str() == "\x78\x56\x34\x12");

  std::optional<int> emptyOpt = std::nullopt;
  io::writeBytes(os, emptyOpt);
  REQUIRE(os.str() == "\x78\x56\x34\x12");
}

TEST_CASE("can write tuples", "[io]") {
  std::ostringstream os{};
  std::tuple<int, std::string, int> t{0x12345678, "hello", 0xabcdef01};
  io::writeBytes(os, t);
  REQUIRE(os.str() == "\x78\x56\x34\x12hello\x01\xef\xcd\xab");
}