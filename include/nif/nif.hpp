#ifndef OPENOBLIVION_NIF_NIF_HPP
#define OPENOBLIVION_NIF_NIF_HPP

#include "nif/compound.hpp"
#include <array>
#include <istream>
#include <iostream>
#include <optional>
#include <string>

namespace nif {

class NifModel {
 public:
  using Copyright = std::array<std::string, 3>;

 private:
  Version version{};
  std::size_t numBlocks{};
  std::vector<std::string> blockTypes{};
  std::size_t numGroups{};
  std::vector<uint32_t> groups{};

 public:
  explicit NifModel(std::istream &is);
};

} // namespace nif

#endif //OPENOBLIVION_NIF_NIF_HPP
