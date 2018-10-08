#include "record/io.hpp"
#include "record/group.hpp"

std::ostream &record::operator<<(std::ostream &os, const Group &grp) {
  os.write(grp.type.data(), 4);
  os.write(reinterpret_cast<const char *>(&grp.groupSize), 4);
  os.write(reinterpret_cast<const char *>(&grp.label), 4);
  os.write(reinterpret_cast<const char *>(&grp.groupType), 4);
  os.write(reinterpret_cast<const char *>(&grp.stamp), 4);

  return os;
}

std::istream &record::operator>>(std::istream &is, Group &grp) {
  char type[5]{};
  if (!io::safeRead(is, type, 4) || grp.type != type) {
    throw RecordNotFoundError(grp.type, type);
  }
  readOrThrow(is, &grp.groupSize, 4, "GRUP");
  readOrThrow(is, &grp.label, 4, "GRUP");
  readOrThrow(is, &grp.groupType, 4, "GRUP");
  readOrThrow(is, &grp.stamp, 4, "GRUP");

  return is;
}

std::optional<record::Group::GroupType> record::peekGroupType(std::istream &is) {
  // Jump back
  char type[5]{};
  is.read(type, 4);
  if (std::string("GRUP") != type) {
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

