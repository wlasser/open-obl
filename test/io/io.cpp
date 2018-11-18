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

TEST_CASE("can read trivial types", "[io]") {
  std::istringstream is1("\x78\x56\x34\x12s", std::ios_base::binary);
  int i{};
  io::readBytes(is1, i);
  REQUIRE(i == 0x12345678);

  std::istringstream is2("x"s, std::ios_base::binary);
  EmptyType empty{};
  io::readBytes(is2, empty);
  // Check stream has not advanced
  REQUIRE(is2.peek() == 'x');

  std::istringstream is3("h\0\0\0\x78\x56\x34\x12"s, std::ios_base::binary);
  PaddedType padded{};
  io::readBytes(is3, padded);
  REQUIRE(padded.c == 'h');
  REQUIRE(padded.i == 0x12345678);
}

TEST_CASE("can write strings", "[io]") {
  std::ostringstream os{};
  const std::string s{"hello, world"};
  io::writeBytes(os, s);
  REQUIRE(os.str() == "hello, world\0"s);
}

TEST_CASE("can read strings", "[io]") {
  std::istringstream is("hello, world\0x"s, std::ios_base::binary);
  std::string s{};
  io::readBytes(is, s);
  REQUIRE(s == "hello, world"s);
  REQUIRE(is.peek() == 'x');
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

TEST_CASE("can read pairs", "[io]") {
  std::istringstream is("\x78\x56\x34\x12hello, world\0x"s,
                        std::ios_base::binary);
  std::pair<int, std::string> p{};
  io::readBytes(is, p);
  REQUIRE(std::get<0>(p) == 0x12345678);
  REQUIRE(std::get<1>(p) == "hello, world"s);
  REQUIRE(is.peek() == 'x');
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

TEST_CASE("can read optionals", "[io]") {
  // Whether or not to read an optional is decided extrinsically
  std::istringstream is("\x78\x56\x34\x12", std::ios_base::binary);
  std::optional<int> opt{};
  io::readBytes(is, opt);
  REQUIRE(opt.has_value());
  REQUIRE(opt.value() == 0x12345678);
}

TEST_CASE("can write tuples", "[io]") {
  std::ostringstream os{};
  std::tuple<int, std::string, int> t{0x12345678, "hello", 0xabcdef01};
  io::writeBytes(os, t);
  REQUIRE(os.str() == "\x78\x56\x34\x12hello\0\x01\xef\xcd\xab"s);
}

TEST_CASE("can read tuples", "[io]") {
  std::istringstream is("\x78\x56\x34\x12hello\0\x01\xef\xcd\xab"s,
                        std::ios_base::binary);
  std::tuple<int, std::string, int> t{};
  io::readBytes(is, t);
  REQUIRE(std::get<0>(t) == 0x12345678);
  REQUIRE(std::get<1>(t) == "hello"s);
  REQUIRE(std::get<2>(t) == 0xabcdef01);
}

TEST_CASE("can write arrays", "[io]") {
  std::ostringstream os{};
  std::array<int, 2> arr{0x01, 0x12345678};
  io::writeBytes(os, arr);
  REQUIRE(os.str() == "\x01\x00\x00\x00\x78\x56\x34\x12"s);
}

TEST_CASE("can read arrays", "[io]") {
  std::istringstream is("\x01\x00\x00\x00\x78\x56\x34\x12"s,
                        std::ios_base::binary);
  std::array<int, 2> arr{};
  io::readBytes(is, arr);
  REQUIRE(arr[0] == 0x00000001);
  REQUIRE(arr[1] == 0x12345678);
}
