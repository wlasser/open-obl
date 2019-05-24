#ifndef OPENOBL_WRLD_HPP
#define OPENOBL_WRLD_HPP

#include "fs/path.hpp"
#include "math/conversions.hpp"
#include "record/formid.hpp"
#include <boost/multi_array.hpp>

namespace oo {

/// Coordinates of an `oo::ExteriorCell` in a `World`.
using CellIndex = qvm::vec<int32_t, 2>;

/// Get the coordinates of the `oo::ExteriorCell` containing the given position.
/// \remark `x` and `y` are measured in BS units.
CellIndex getCellIndex(float x, float y) noexcept;

using CellGrid = boost::multi_array<oo::BaseId, 2>;

using CellGridView = boost::multi_array<oo::BaseId,
                                        2>::const_array_view<2>::type;

/// Coordinates of a distant cell chunk in a `World`.
using ChunkIndex = qvm::vec<int32_t, 2>;

/// Get the coordinates of the distant cell chunk containing the given position.
/// \remark `x` and `y` are measured in BS units.
ChunkIndex getChunkIndex(float x, float y) noexcept;

std::string getChunkBaseName(oo::BaseId wrldId, oo::ChunkIndex chunkIndex);

oo::Path getChunkMeshPath(oo::BaseId wrldId, oo::ChunkIndex chunkIndex);
oo::Path getChunkDiffusePath(oo::BaseId wrldId, oo::ChunkIndex chunkIndex);
oo::Path getChunkNormalPath(oo::BaseId wrldId, oo::ChunkIndex chunkIndex);

} // namespace oo

#endif //OPENOBL_WRLD_HPP
