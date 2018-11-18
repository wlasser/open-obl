#include <catch.hpp>
#include <optional>
#include <sstream>
#include <variant>
#include "io/io.hpp"

using std::string_literals::operator ""s;

TEST_CASE("can write trivial types", "[io]") {
  std::ostringstream os{};
  const int i{0x12345678};
  io::writeBytes(os, i);
  REQUIRE(os.str() == "\x78\x56\x34\x12");
}

TEST_CASE("can write strings", "[io]") {
  std::ostringstream os{};
  const std::string s{"hello, world"};
  io::writeBytes(os, s);
  REQUIRE(os.str() == "hello, world\0"s);
}

TEST_CASE("can write pairs", "[io]") {
  std::ostringstream os{};
  const std::pair<int, std::string> p{0x12345678, "hello, world"};
  io::writeBytes(os, p);
  REQUIRE(os.str() == "\x78\x56\x34\x12hello, world\0"s);
}

TEST_CASE("can write optionals", "[io]") {
  std::ostringstream os{};
  const std::optional<int> opt{0x12345678};
  io::writeBytes(os, opt);
  REQUIRE(os.str() == "\x78\x56\x34\x12");

  const std::optional<int> emptyOpt{std::nullopt};
  io::writeBytes(os, emptyOpt);
  REQUIRE(os.str() == "\x78\x56\x34\x12");
}

TEST_CASE("can write tuples", "[io]") {
  std::ostringstream os{};
  std::tuple<int, std::string, int> t{0x12345678, "hello", 0xabcdef01};
  io::writeBytes(os, t);
  REQUIRE(os.str() == "\x78\x56\x34\x12hello\0\x01\xef\xcd\xab"s);
}
