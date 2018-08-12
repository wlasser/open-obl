#ifndef ESP_HPP
#define ESP_HPP

#include <istream>
#include <ostream>
#include <iostream>
#include <optional>
#include "formid.hpp"
#include "records.hpp"

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
  typedef struct {
    uint16_t y;
    uint16_t x;
  } Grid;
  union Label {
    char recordType[4];
    FormID parent;
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
  const std::string type = "GRUP";
  // Size of the group, including the data in this header
  uint32_t groupSize = 0;
  // Generally parent information or record type, depends on type
  Label label{};
  GroupType groupType = GroupType::Top;
  // Date stamp
  uint32_t stamp = 0;
};

std::ostream &operator<<(std::ostream &, const Group &);
std::istream &operator>>(std::istream &, Group &);

class GroupError : std::runtime_error {
 public:
  explicit GroupError(const std::string &what) : std::runtime_error(what) {}
};
}

class Esp {
 public:

  explicit Esp(std::istream &);

  template<class T>
  bool parseRecord(std::istream &is) {
    T rec;
    is >> rec;
    bool isOk = is.rdstate() == std::ios::goodbit;
    if (!isOk) {
      std::clog << "read failed: ";
      switch (is.rdstate()) {
        case std::ios::goodbit: std::clog << "goodbit\n";
          break;
        case std::ios::badbit: std::clog << "badbit\n";
          break;
        case std::ios::failbit: std::clog << "failbit\n";
          break;
        case std::ios::eofbit: std::clog << "eofbit\n";
          break;
        default: std::clog << "unknown\n";
          break;
      }
    }
    std::cout << rec << "\n<CLEAR>\n";
    return isOk;
  }

  void skipRecord(std::istream &is) {
    char type[4]{};
    is.read(type, 4);
    uint32_t size;
    is.read(reinterpret_cast<char *>(&size), 4);
    //std::clog << "Skipping " << type << " (" << 12 + size << ") bytes\n";
    is.seekg(12 + size, std::istream::cur);
  }

  std::optional<record::Group::GroupType> peekGroupType(std::istream &is) {
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

  void parseCell(std::istream &is) {
    using namespace record;
    // Expect a CELL record
    parseRecord<record::CELL>(is);

    // Expect a cell children group, though there exist empty cells
    // like Hackdirt so this is optional.
    if (peekGroupType(is) == Group::GroupType::CellChildren) {
      Group cellChildren;
      is >> cellChildren;

      std::string recordType;

      // Persistent children group
      if (peekGroupType(is)
          == Group::GroupType::CellPersistentChildren) {
        Group persistentChildren;
        is >> persistentChildren;

        // Expect REFR, ACHR, or ACRE records describing the
        // persistent objects, actors, and creatures in the cell,
        // respectively. This may be empty.
        while ((recordType = peekRecordType(is)) == "REFR"
            || recordType == "ACHR" || recordType == "ACRE") {
          // TODO: REFR, ACHR, ACRE
          skipRecord(is);
        }
      }

      // Visible when distant children group
      if (peekGroupType(is)
          == Group::GroupType::CellVisibleDistantChildren) {
        Group visibleDistantChildren;
        is >> visibleDistantChildren;

        // Expect as CellPersistentChildren
        while ((recordType = peekRecordType(is)) == "REFR"
            || recordType == "ACHR" || recordType == "ACRE") {
          // TODO: REFR, ACHR, ACRE
          skipRecord(is);
        }
      }

      // Temporary children group
      if (peekGroupType(is)
          == Group::GroupType::CellTemporaryChildren) {
        Group temporaryChildren;
        is >> temporaryChildren;

        // Unsure if PGRD is usually optional or not, but sometimes
        // this entire group is empty (e.g. ImperialSewerSystemTG11)
        if (peekRecordType(is) == "PGRD") {
          // TODO: PGRD
          skipRecord(is);
        }

        // Expect as CellPersistentChildren until next GRUP or CELL
        while ((recordType = peekRecordType(is)) == "REFR"
            || recordType == "ACHR" || recordType == "ACRE") {
          // TODO: REFR, ACHR, ACRE
          skipRecord(is);
        }
      }
    }
  }
};

#endif // ESP_HPP
