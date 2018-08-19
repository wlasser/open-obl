#include "nif/nif.hpp"

nif::NifModel::NifModel(std::istream &is) {
  // To instantiate a header we need a version, but we don't know the version
  // unless we've read the header. We therefore read the first line of the
  // header independently, grab the version, then jump back and read the thing
  basic::HeaderString headerVersion{};
  is >> headerVersion;

  // Sanity check; header should start with 'Gamebryo' or 'NetImmerse'
  auto firstWordPos = headerVersion.str.find_first_of(' ');
  if (firstWordPos == std::string::npos || firstWordPos == 0) {
    throw std::runtime_error("Invalid nif header");
  }
  auto formatName = headerVersion.str.substr(0, firstWordPos);
  if (formatName != "Gamebryo" && formatName != "NetImmerse") {
    throw std::runtime_error(std::string(
        "Invalid nif header. Expected 'NetImmerse' or 'Gamebryo', found ")
                                 .append(formatName));
  }

  // Now proceed with finding the version
  auto lastWordPos = headerVersion.str.find_last_of(' ');
  auto versionString =
      headerVersion.str.substr(lastWordPos + 1, std::string::npos);
  version = verOf(versionString.c_str(), versionString.length());
  is.seekg(0);

  compound::Header header{version};
  is >> header;

  if (header.userVer) {
    userVersion = header.userVer.value();
  }
  if (header.bsStreamHeader.userVersion2 != 0) {
    userVersion2 = header.bsStreamHeader.userVersion2;
  }

  if (!header.numBlocks || header.numBlocks.value() == 0) {
    // File is empty, we can stop (technically this is ok for sufficiently low
    // versions, but we do not support those)
    return;
  }
  auto numBlocks = header.numBlocks.value();

  if (header.numBlockTypes && header.blockTypes) {
    blockTypes.reserve(header.numBlockTypes.value());
    for (const auto &blockType : header.blockTypes.value()) {
      auto blockTypeString = std::string(blockType.value.begin(),
                                         blockType.value.end());
      blockTypes.push_back(blockTypeString);
    }
  } else {
    // In this case, the block types are written directly before their data,
    // similar to an esp file.
    // TODO: This is not an error
    //throw std::runtime_error("nif file has no block types");
  }
  if (header.numGroups && header.groups) {
    for (const auto &group : header.groups.value()) {
      groups.push_back(group);
    }
  } else {
    // Unsure precisely what groups are used for, so this is not an error
  }
}