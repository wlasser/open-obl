#include "io/io.hpp"
#include <catch2/catch.hpp>
#include <array>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>

using std::string_literals::operator ""s;
using std::string_view_literals::operator ""sv;

struct EmptyType {};
struct PaddedType {
  char c{};
  int i{};
};

TEST_CASE("can write trivial types", "[io]") {
  std::ostringstream os1{};
  const int i{0x12345678};
  io::writeBytes(os1, i);
  REQUIRE(os1.str() == "\x78\x56\x34\x12"s);

  std::ostringstream os2{};
  EmptyType empty{};
  io::writeBytes(os2, empty);
  REQUIRE(os2.str().empty());

  std::ostringstream os3{};
  PaddedType padded{'h', 0x12345678};
  io::writeBytes(os3, padded);
  std::string os3str{os3.str()};
  // Padding bits are implementation defined, we have to check each component
  REQUIRE(os3str.length() == 8u);
  REQUIRE(os3str[0] == 'h');
  REQUIRE(os3str.substr(4, 4) == "\x78\x56\x34\x12"s);
}

TEST_CASE("can write strings", "[io]") {
  std::ostringstream os{};
  const std::string s{"hello, world"};
  io::writeBytes(os, s);
  REQUIRE(os.str() == "hello, world\0"s);
}

TEST_CASE("can write string_view", "[io]") {
  std::ostringstream os{};
  const std::string_view sv{"hello"};
  io::writeBytes(os, sv);
  REQUIRE(os.str() == "hello"s);
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
  REQUIRE(os.str() == "\x78\x56\x34\x12"s);

  const std::optional<int> emptyOpt{std::nullopt};
  io::writeBytes(os, emptyOpt);
  REQUIRE(os.str() == "\x78\x56\x34\x12"s);
}

TEST_CASE("can write tuples", "[io]") {
  std::ostringstream os{};
  std::tuple<int, std::string, int> t{0x12345678, "hello", 0xabcdef01};
  io::writeBytes(os, t);
  REQUIRE(os.str() == "\x78\x56\x34\x12hello\0\x01\xef\xcd\xab"s);
}

TEST_CASE("can write arrays", "[io]") {
  std::ostringstream os{};
  std::array<int, 2> arr{0x01, 0x12345678};
  io::writeBytes(os, arr);
  REQUIRE(os.str() == "\x01\x00\x00\x00\x78\x56\x34\x12"s);
}
