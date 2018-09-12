#ifndef OPENOBLIVION_ESP_HPP
#define OPENOBLIVION_ESP_HPP

#include "io/io.hpp"
#include "records.hpp"
#include "record/group.hpp"
#include "record/io.hpp"
#include "record/rec_of.hpp"
#include <array>
#include <istream>
#include <ostream>
#include <string>

namespace esp {

// Read an entire esp file from the beginning, delegating the actual reading
// to the `processor`. The `processor` should have a function template
// template<class T> void readRecord(std::istream &is);
// which reads the next record, having type `T`, from the stream. `readEsp`
// guarantees that `is` will always be positioned before a record of type `T`,
// and requires that `is` be positioned immediately after that record when
// `processor.readRecord` returns.
// Broadly speaking it is expected that Processors will want to either read the
// entire record and process/cache it, or will want to make note of the record
// location and skip it until it is needed in the future. For these purposes
// one should use the `record::readRecord` and `record::skipRecord` functions
// respectively, then process the returned record/header. By using these
// functions one guarantees that `is` will be positioned correctly on return.
template<class Processor>
void readEsp(std::istream &is, Processor &processor);

template<class Processor>
void readCellChildren(std::istream &is, Processor &processor);

template<class Processor>
void parseCellChildrenBlock(std::istream &is, Processor &processor);

template<class Processor>
void readEsp(std::istream &is, Processor &processor) {
  // First is always a TES4 record
  record::peekOrThrow(is, "TES4");
  processor.template readRecord<record::TES4>(is);

  // Now we expect a collection of top groups
  while (record::peekRecordType(is) == "GRUP") {
    record::Group topGrp;
    is >> topGrp;

    if (topGrp.groupType != record::Group::GroupType::Top) {
      throw record::GroupError("Expected TOP GRUP at top level");
    }

    // Get the record type of the top group. This is a little verbose because
    // we need an int to switch over.
    using namespace record;
    const auto &rt = topGrp.label.recordType;
    std::array<char, 4> recTypeArr = {rt[0], rt[1], rt[2], rt[3]};
    auto recType = recOf(recTypeArr);

    // All top groups except CELL, WRLD, and DIAL contain only records of the
    // same type as the group.
    switch (recType) {
      // Invalid record
      case 0: throw RecordNotFoundError("a record", rt);
      case "CELL"_rec: {
        // Expect a series of InteriorCellBlock groups
        while (peekGroupType(is) == Group::GroupType::InteriorCellBlock) {
          Group interiorCellBlock;
          is >> interiorCellBlock;

          // Expect a series of InteriorCellSubblock groups
          while (peekGroupType(is) == Group::GroupType::InteriorCellSubblock) {
            Group interiorCellSubblock;
            is >> interiorCellSubblock;

            // Expect a series of cells
            while (peekRecordType(is) == "CELL") {
              processor.template readRecord<record::CELL>(is);

              // Expect a cell children group, though there exist empty cells,
              // like Hackdirt, so this is optional.
              if (peekGroupType(is) == Group::GroupType::CellChildren) {
                readCellChildren(is, processor);
              }
            }
          }
        }
        break;
      }
      case "WRLD"_rec: skipGroup(is);
        break;
      case "DIAL"_rec: skipGroup(is);
        break;
      default: {
        // Otherwise we expect a block of records all of the same type
        while (peekRecordType(is) == topGrp.label.recordType) {
          switch (recType) {
            case "GMST"_rec: processor.template readRecord<record::GMST>(is);
              break;
            case "GLOB"_rec: processor.template readRecord<record::GLOB>(is);
              break;
            case "CLAS"_rec: processor.template readRecord<record::CLAS>(is);
              break;
            case "FACT"_rec: processor.template readRecord<record::FACT>(is);
              break;
            case "HAIR"_rec: processor.template readRecord<record::HAIR>(is);
              break;
            case "EYES"_rec: processor.template readRecord<record::EYES>(is);
              break;
            case "RACE"_rec: processor.template readRecord<record::RACE>(is);
              break;
            case "SOUN"_rec: processor.template readRecord<record::SOUN>(is);
              break;
            case "SKIL"_rec: processor.template readRecord<record::SKIL>(is);
              break;
            case "MGEF"_rec: processor.template readRecord<record::MGEF>(is);
              break;
            case "LTEX"_rec: processor.template readRecord<record::LTEX>(is);
              break;
            case "STAT"_rec: processor.template readRecord<record::STAT>(is);
              break;
            case "ALCH"_rec: processor.template readRecord<record::ALCH>(is);
              break;
            default: (void) skipRecord(is);
              break;
          }
        }
      }
    }
  }
}

template<class Processor>
void readCellChildren(std::istream &is, Processor &processor) {
  using namespace record;

  Group cellChildren;
  is >> cellChildren;

  if (peekGroupType(is) == Group::GroupType::CellPersistentChildren) {
    Group persistentChildren;
    is >> persistentChildren;

    parseCellChildrenBlock(is, processor);
  }

  if (peekGroupType(is) == Group::GroupType::CellVisibleDistantChildren) {
    Group visibleDistantChildren;
    is >> visibleDistantChildren;

    parseCellChildrenBlock(is, processor);
  }

  if (peekGroupType(is) == Group::GroupType::CellTemporaryChildren) {
    Group temporaryChildren;
    is >> temporaryChildren;

    // Unsure if PGRD is usually optional or not, but sometimes this entire
    // group is empty e.g. ImperialSewerSystemTG11
    if (peekRecordType(is) == "PGRD") {
      // TODO: PGRD
      (void) skipRecord(is);
    }

    parseCellChildrenBlock(is, processor);
  }
}

template<class Processor>
void parseCellChildrenBlock(std::istream &is, Processor &processor) {
  using namespace record;
  for (auto type = peekRecordType(is);; type = peekRecordType(is)) {
    if (type == "REFR") {
      processor.template readRecord<record::REFR>(is);
    } else if (type == "ACHR") {
      (void) skipRecord(is);
    } else if (type == "ACRE") {
      (void) skipRecord(is);
    } else {
      return;
    }
  }
}

} // namespace esp

#endif // OPENOBLIVION_ESP_HPP
