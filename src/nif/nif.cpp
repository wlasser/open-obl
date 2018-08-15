#include "nif/nif.hpp"

nif::NifModel::NifModel(std::istream &is) {
  // To instantiate a header we need a version, but we don't know the version
  // unless we've read the header. We therefore read the first line of the
  // header independently, grab the version, then jump back and read the thing
  auto startPos = is.tellg();
  basic::HeaderString headerVersion{};
  is >> headerVersion;
  auto lastWordPos = headerVersion.str.find_last_of(' ');
  auto versionString =
      headerVersion.str.substr(lastWordPos + 1, std::string::npos);
  version = verOf(versionString.c_str(), versionString.length());
  is.seekg(startPos);

  compound::Header header{version};
  is >> header;

  // TODO: Optionals!
  numBlocks = header.numBlocks.value();
  blockTypes.reserve(header.numBlockTypes.value());
  for (const auto &blockType : header.blockTypes.value()) {
    auto blockTypeString = std::string(blockType.value.data());
    blockTypes.push_back(blockTypeString);
  }
  for (const auto &group : header.groups.value()) {
    groups.push_back(group);
  }
}