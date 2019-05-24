#ifndef OPENOBL_RECORD_RECORD_HEADER_HPP
#define OPENOBL_RECORD_RECORD_HEADER_HPP

#include "record/formid.hpp"
#include <cstdint>

namespace record {

// RecordFlag needs to be standard layout and 'look like' a uint32_t, which
// a Bitflag doesn't.
enum class RecordFlag : uint32_t {
  None = 0,
  ESM = 0x00000001,
  Deleted = 0x00000020,
  CastsShadows = 0x00000200,
  QuestOrPersistent = 0x00000400,
  InitiallyDisabled = 0x00000800,
  Ignored = 0x00001000,
  VisibleWhenDistant = 0x00008000,
  OffLimits = 0x00020000,
  Compressed = 0x00040000,
  CantWait = 0x00080000
};
constexpr RecordFlag operator&(RecordFlag a, RecordFlag b) noexcept {
  return static_cast<RecordFlag>(static_cast<uint32_t>(a)
      & static_cast<uint32_t>(b));
}

// It is useful to be able to read the header of a record, but not read the
// record itself.
struct RecordHeader {
  using Flag = RecordFlag;
  uint32_t type{};
  uint32_t size{};
  Flag flags{Flag::None};
  oo::FormId id{};
  // We never use this, so omit it to get a nice 16-byte struct
  //uint32_t versionControlInfo{};
};

} // namespace record

#endif //OPENOBL_RECORD_RECORD_HEADER_HPP
