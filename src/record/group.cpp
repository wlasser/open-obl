#include "record/io.hpp"
#include "record/group.hpp"
#include <istream>

std::ostream &record::operator<<(std::ostream &os, const Group &grp) {
  os.write(grp.type.data(), 4);
  os.write(reinterpret_cast<const char *>(&grp.groupSize), 4);
  os.write(reinterpret_cast<const char *>(&grp.label), 4);
  os.write(reinterpret_cast<const char *>(&grp.groupType), 4);
  os.write(reinterpret_cast<const char *>(&grp.stamp), 4);

  return os;
}

std::istream &record::operator>>(std::istream &is, Group &grp) {
  std::string type(4, ' ');
  if (!io::safeRead(is, type.data(), 4) || grp.type != type) {
    throw RecordNotFoundError(grp.type, type);
  }
  io::readBytes(is, grp.groupSize);
  io::readBytes(is, grp.label);
  io::readBytes(is, grp.groupType);
  io::readBytes(is, grp.stamp);

  return is;
}

std::optional<record::Group::GroupType> record::peekGroupType(std::istream &is) {
  // Jump back
  std::string type(4, ' ');
  is.read(type.data(), 4);
  if (type != "GRUP") {
    is.seekg(-4, std::istream::cur);
    return std::nullopt;
  }
  // Skip past groupSize and label
  is.seekg(8, std::istream::cur);
  record::Group::GroupType groupType{};
  is.read(reinterpret_cast<char *>(&groupType), 4);
  is.seekg(-16, std::istream::cur);
  return groupType;
}

