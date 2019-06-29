#ifndef OPENOBL_RECORD_GROUP_HPP
#define OPENOBL_RECORD_GROUP_HPP

#include "record/formid.hpp"
#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string_view>

namespace record {

// Unlike (sub)records, groups do not store data and instead function as
// separators in the ESP to delimit blocks of data. Almost all top level groups
// precede a block of records matching the type of group. The exceptions to this
// are the CELL, WRLD, and DIAL groups which each have child groups of differing
// types, including data that is not a type of record.
class Group {
 private:

  friend std::ostream &operator<<(std::ostream &, const Group &);
  friend std::istream &operator>>(std::istream &, Group &);

 public:
  struct Grid {
    uint16_t y;
    uint16_t x;
  };
  union Label {
    char recordType[4];
    oo::FormId parent;
    uint32_t blockNumber;
    uint32_t subblockNumber;
    Grid grid;
  };
  enum class GroupType : int32_t {
    Top = 0,
    WorldChildren,
    InteriorCellBlock,
    InteriorCellSubblock,
    ExteriorCellBlock,
    ExteriorCellSubblock,
    CellChildren,
    TopicChildren,
    CellPersistentChildren,
    CellTemporaryChildren,
    CellVisibleDistantChildren
  };
  constexpr static inline std::string_view type{"GRUP"};
  // Size of the group, including the data in this header
  uint32_t groupSize{};
  // Generally parent information or record type, depends on type
  Label label{};
  GroupType groupType{GroupType::Top};
  // Date stamp
  uint32_t stamp{};
};

std::ostream &operator<<(std::ostream &, const Group &);
std::istream &operator>>(std::istream &, Group &);

class GroupError : public virtual std::runtime_error {
 public:
  explicit GroupError(const std::string &what) : std::runtime_error(what) {}
};

std::optional<record::Group::GroupType>
peekGroupType(std::istream &is) noexcept;

} // namespace record

#endif //OPENOBL_RECORD_GROUP_HPP
