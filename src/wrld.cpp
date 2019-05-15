#include "wrld.hpp"

namespace oo {

oo::CellIndex getCellIndex(float x, float y) noexcept {
  return {
      static_cast<int32_t>(std::floor(x / oo::unitsPerCell<float>)),
      static_cast<int32_t>(std::floor(y / oo::unitsPerCell<float>))
  };
}

oo::ChunkIndex getChunkIndex(float x, float y) noexcept {
  return {
      static_cast<int32_t>(std::floor(x / oo::unitsPerChunk<float>)),
      static_cast<int32_t>(std::floor(y / oo::unitsPerChunk<float>))
  };
}

std::string getChunkBaseName(oo::BaseId wrldId, oo::ChunkIndex chunkIndex) {
  const int x{qvm::X(chunkIndex)}, y{qvm::Y(chunkIndex)};

  //C++20: Use std::format
  // Can't use wrldId.string() since decimal is needed.
  std::string result(std::to_string(static_cast<oo::FormId>(wrldId)));
  result.reserve(6u + 1u + 2u + 1u + 2u + 3u);
  result.push_back('.');
  result.append(x == 0 ? "00" : std::to_string(32 * x));
  result.push_back('.');
  result.append(y == 0 ? "00" : std::to_string(32 * y));
  result.append(".32");

  return result;
}

oo::Path getChunkMeshPath(oo::BaseId wrldId, oo::ChunkIndex chunkIndex) {
  oo::Path filename{oo::getChunkBaseName(wrldId, chunkIndex).append(".nif")};
  return oo::Path{"meshes/landscape/lod"} / filename;
}

oo::Path getChunkDiffusePath(oo::BaseId wrldId, oo::ChunkIndex chunkIndex) {
  oo::Path filename{oo::getChunkBaseName(wrldId, chunkIndex).append(".dds")};
  return oo::Path{"textures/landscapelod/generated"} / filename;
}

oo::Path getChunkNormalPath(oo::BaseId wrldId, oo::ChunkIndex chunkIndex) {
  oo::Path filename{oo::getChunkBaseName(wrldId, chunkIndex).append("_fn.dds")};
  return oo::Path{"textures/landscapelod/generated"} / filename;
}

} // namespace oo