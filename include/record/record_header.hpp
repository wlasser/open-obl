#ifndef OPENOBLIVION_RECORD_RECORD_HEADER_HPP
#define OPENOBLIVION_RECORD_RECORD_HEADER_HPP

#include "formid.hpp"
#include <cstdint>

namespace record {

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

// It is useful to be able to read the header of a record, but not read the
// record itself.
struct RecordHeader {
  using Flag = RecordFlag;
  uint32_t type{};
  uint32_t size{};
  Flag flags{Flag::None};
  FormID id{};
  // We never use this, so omit it to get a nice 16-byte struct
  //uint32_t versionControlInfo{};
};

} // namespace record

#endif //OPENOBLIVION_RECORD_RECORD_HEADER_HPP
