#include "io/memstream.hpp"
#include <catch2/catch.hpp>
#include <cstdint>
#include <vector>

TEST_CASE("can construct and read from memstream", "[io]") {
  std::vector<uint8_t> bytes{1, 2, 3, 4, 5, 6, 7, 8};
  io::memstream m(bytes.data(), bytes.size());

  REQUIRE(m.peek() == 1);
  REQUIRE(m.get() == 1);
  REQUIRE(m.peek() == 2);

  int i{};
  m.read(reinterpret_cast<char *>(&i), 4u);
  REQUIRE(i == 0x05040302);

  m.unget();
  REQUIRE(m.peek() == 5);

  m.seekg(-1, std::ios_base::cur);
  REQUIRE(m.peek() == 4);

  m.seekg(2, std::ios_base::cur);
  REQUIRE(m.peek() == 6);

  m.seekg(0, std::ios_base::beg);
  REQUIRE(m.peek() == 1);

  m.seekg(-1, std::ios_base::end);
  REQUIRE(m.peek() == 8);

  int j{};
  m.read(reinterpret_cast<char *>(&j), 4u);
  REQUIRE(m.fail());
  REQUIRE(m.gcount() == 1);
}
