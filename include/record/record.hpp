#ifndef OPENOBLIVION_RECORD_RECORD_HPP
#define OPENOBLIVION_RECORD_RECORD_HPP

#include "formid.hpp"
#include "io/write_bytes.hpp"
#include "io/read_bytes.hpp"
#include "record/exceptions.hpp"
#include "record/record_header.hpp"
#include "record/rec_of.hpp"
#include "record/io.hpp"
#include "record/size_of.hpp"
#include <cstdint>
#include <string>
#include <istream>
#include <ostream>
#include <stdexcept>

namespace record {

// Wrapper for records
template<class T, uint32_t c>
class Record {
 public:
  const std::string type = recOf<c>();
  uint32_t size() const {
    return static_cast<uint32_t>(data.size());
  }
  using Flag = record::RecordFlag;
  Flag flags = Flag::None;
  FormId id = 0;
  // Version control info. This is bugged in the original implementation, with
  // Dec coming before Jan of the same year and not the next one. We ignore it.
  uint32_t versionControlInfo = 0;
  T data;

  Record(const T &t, Flag flags, FormId id, uint32_t versionControlInfo) :
      flags(flags), id(id), versionControlInfo(versionControlInfo), data(t) {}
  Record() : Record(T(), Flag::None, 0, 0) {}
};

// Read and write records through a stream. Like the subrecord stream IO
// functions, these should not need to be specialized; all the
// specialization is done in raw::read and raw::write.
template<class T, uint32_t c>
std::ostream &operator<<(std::ostream &os, const Record<T, c> &record) {
  uint32_t size = record.size();
  os.write(record.type.data(), 4);
  io::writeBytes(os, size);
  io::writeBytes(os, record.flags);
  io::writeBytes(os, record.id);
  io::writeBytes(os, record.versionControlInfo);
  raw::write(os, record.data, size);

  return os;
}

template<class T, uint32_t c>
std::istream &operator>>(std::istream &is, Record<T, c> &record) {
  char type[5]{};
  is.read(type, 4);
  if (record.type != type) throw RecordNotFoundError(record.type, type);

  uint32_t size;
  io::readBytes(is, size);
  io::readBytes(is, record.flags);
  io::readBytes(is, record.id);
  io::readBytes(is, record.versionControlInfo);
  raw::read(is, record.data, size);

  return is;
}

} // namespace record

#endif // OPENOBLIVION_RECORD_RECORD_HPP
