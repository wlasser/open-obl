#include "io/string.hpp"
#include <catch2/catch.hpp>
#include <sstream>
#include <string>

using std::string_literals::operator ""s;

TEST_CASE("can read bstring", "[io]") {
  {
    // The 'x' is not part of the string to read.
    std::istringstream is("\x0bHello worldx"s, std::ios_base::binary);
    const std::string s{io::readBString(is)};
    REQUIRE(s == "Hello world");
    REQUIRE(is.peek() == 'x');
  }

  {
    std::istringstream is("\x00x"s, std::ios_base::binary);
    const std::string s{io::readBString(is)};
    REQUIRE(s.empty());
    REQUIRE(is.peek() == 'x');
  }

  {
    const std::string longString(255, 'a');
    std::istringstream is("\xff"s + longString + "x"s, std::ios_base::binary);
    const std::string s{io::readBString(is)};
    // Early-out for easier reading
    REQUIRE(s.length() == longString.length());
    REQUIRE(s == longString);
    REQUIRE(is.peek() == 'x');
  }
}

TEST_CASE("can read bzstring", "[io]") {
  {
    // The 'x' is not part of the string to read.
    std::istringstream is("\x0cHello world\0x"s, std::ios_base::binary);
    const std::string s{io::readBzString(is)};
    REQUIRE(s == "Hello world"s);
    REQUIRE(is.peek() == 'x');
  }

  {
    std::istringstream is("\x01\0x"s, std::ios_base::binary);
    const std::string s{io::readBzString(is)};
    REQUIRE(s.empty());
    REQUIRE(is.peek() == 'x');
  }

  {
    std::istringstream is("\x00x"s, std::ios_base::binary);
    const std::string s{io::readBzString(is)};
    REQUIRE(s.empty());
    REQUIRE(is.peek() == 'x');
  }

  {
    // Length includes null-terminator, so can only fit 254 actual characters.
    const std::string longString(254, 'a');
    std::istringstream is("\xff"s + longString + "\0x"s, std::ios_base::binary);
    const std::string s{io::readBzString(is)};
    // Early-out for easier reading
    REQUIRE(s.length() == longString.length());
    REQUIRE(s == longString);
    REQUIRE(is.peek() == 'x');
  }

  {
    // The 'x' is not part of the string to read.
    std::istringstream is("\x0cHello\0World\0x"s, std::ios_base::binary);
    const std::string s{io::readBzString(is)};
    REQUIRE(s == "Hello\0World"s);
    REQUIRE(is.peek() == 'x');
  }
}