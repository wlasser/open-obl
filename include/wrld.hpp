#ifndef OPENOBLIVION_WRLD_HPP
#define OPENOBLIVION_WRLD_HPP

#include "math/conversions.hpp"
#include "record/formid.hpp"
#include <boost/multi_array.hpp>

namespace oo {

/// Coordinates of an `oo::ExteriorCell` in a `World`.
using CellIndex = qvm::vec<int32_t, 2>;

/// Get the coordinates of the `oo::ExteriorCell` containing the given position.
/// \remark `x` and `y` are measured in BS units.
inline CellIndex getCellIndex(float x, float y) noexcept {
  return {
      static_cast<int32_t>(std::floor(x / oo::unitsPerCell<float>)),
      static_cast<int32_t>(std::floor(y / oo::unitsPerCell<float>))
  };
}

using CellGrid = boost::multi_array<oo::BaseId, 2>;

using CellGridView = boost::multi_array<oo::BaseId,
                                        2>::const_array_view<2>::type;

} // namespace oo

#endif //OPENOBLIVION_WRLD_HPP
