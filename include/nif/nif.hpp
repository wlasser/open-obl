#ifndef OPENOBLIVION_NIF_NIF_HPP
#define OPENOBLIVION_NIF_NIF_HPP

#include "nif/compound.hpp"
#include "nif/niobject.hpp"
#include <array>
#include <istream>
#include <iostream>
#include <optional>
#include <string>

namespace nif {

class NifModel {
 private:
  using Copyright = std::array<std::string, 3>;

  Version version{};
  std::optional<Version> userVersion{};
  std::optional<Version> userVersion2{};

  std::vector<std::unique_ptr<nif::NiObject>> blocks{};
  std::vector<std::string> blockTypes{};
  std::size_t numGroups{};
  std::vector<uint32_t> groups{};

 public:
  explicit NifModel(std::istream &is);

  Version getVersion() {
    return version;
  }

  std::optional<Version> getUserVersion() {
    return userVersion;
  }

  std::optional<Version> getUserVersion2() {
    return userVersion2;
  }
};

} // namespace nif

#endif //OPENOBLIVION_NIF_NIF_HPP
