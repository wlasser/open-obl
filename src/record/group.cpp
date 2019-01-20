#include "record/exceptions.hpp"
#include "record/io.hpp"
#include "record/group.hpp"
#include <gsl/gsl>

std::ostream &record::operator<<(std::ostream &os, const Group &grp) {
  io::writeBytes(os, Group::type);
  io::writeBytes(os, grp.groupSize);
  io::writeBytes(os, grp.label);
  io::writeBytes(os, grp.groupType);
  io::writeBytes(os, grp.stamp);

  return os;
}

std::istream &record::operator>>(std::istream &is, Group &grp) {
  std::array<char, 4> type{};
  io::readBytes(is, type);

  const std::string_view typeView(type.data(), 4);
  if (typeView != Group::type) {
    throw RecordNotFoundError(Group::type, typeView);
  }

  io::readBytes(is, grp.groupSize);
  io::readBytes(is, grp.label);
  io::readBytes(is, grp.groupType);
  io::readBytes(is, grp.stamp);

  return is;
}

std::optional<record::Group::GroupType>
record::peekGroupType(std::istream &is) noexcept {
  // Ensure that the stream is always returned to its original position.
  const auto startPos{is.tellg()};
  const auto seekGuard = gsl::finally([&]() {
    is.clear();
    is.seekg(startPos, std::ios_base::beg);
  });

  std::array<char, 4> type{};
  try {
    io::readBytes(is, type);
  } catch (const io::IOReadError &e) {
    return std::nullopt;
  }
  const std::string_view typeView(type.data(), 4);
  if (typeView != Group::type) return std::nullopt;

  // Skip past groupSize and label
  is.seekg(8, std::istream::cur);
  record::Group::GroupType groupType{};
  try {
    io::readBytes(is, groupType);
  } catch (io::IOReadError &e) {
    return std::nullopt;
  }

  return groupType;
}

