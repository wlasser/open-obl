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

// The functions in this file handle reading an esp (or esm) file.
// Since these files can be quite large, it is not necessarily practical to
// load the entire file into memory. Broadly speaking, the global parsing of the
// file is handled by these functions, whereas the local parsing is delegated to
// an instance of a Processor type.
//
// A Processor is required to implement a function template
// ```
// template<class T> void readRecord(std::istream &is)
// ```
// for each type `T` arising from the class template `record::Record`.
// Instantiations of this function template are invoked by the caller when a
// record of type `T` is encountered in the esp file, with `is` positioned at
// the beginning of the record header. Expect in the cases noted below,
// `readRecord` should leave `is` one byte after the end of the record upon
// returning. To ensure that this happens, it is recommended that the caller use
// the functions `record::readRecord` and `record::skipRecord`. These correctly
// advance `is` and return a `record::Record` or `record::RecordHeader`
// respectively.
//
// If a group in the esp file is being read, then `readRecord` is guaranteed to
// be invoked for every record in the group in the order that it appears,
// expect in the groups CELL, WRLD, and DIAL. Some of the entries in these
// groups contain a list of child groups, which the Processor may handle
// differently.
//
// When a CELL record appears, it is (almost) always followed by a CellChildren
// subgroup. It is expected that `readRecord<CELL>` read (or skip) both the CELL
// record and all its children. The `readCellChildren` method assists with this.
namespace esp {

// Read an entire esp file from the beginning, delegating the actual reading
// to the `processor`
template<class Processor>
void readEsp(std::istream &is, Processor &processor);

// This function reads the CellChildren subgroup following a CELL record.
// The reading of the PersistentChildren, VisibleDistantChildren, and
// TemporaryChildren are delegated to the corresponding processors. The first
// two processors must be able to read REFR, ACHR, and ACRE. The third processor
// must be able to read REFR, ACHR, ACRE, and PGRD records. If the parent CELL
// is part of an exterior cell, then the third processor must also be able to
// read LAND records.
// Note that in rare cases, a CELL may not have any children, in which case
// this function does nothing.
template<class PersistentProcessor,
    class VisibleDistantProcessor,
    class TemporaryProcessor>
void readCellChildren(std::istream &is,
                      PersistentProcessor &persistentProcessor,
                      VisibleDistantProcessor &visibleDistantProcessor,
                      TemporaryProcessor &temporaryProcessor);

// Read an individual subgroup of a CellChildren subgroup, namely a
// PersistentChildren, VisibleDistantChildren, or TemporaryChildren subgroup.
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
            case "LIGH"_rec: processor.template readRecord<record::LIGH>(is);
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

template<class PersistentProcessor,
    class VisibleDistantProcessor,
    class TemporaryProcessor>
void readCellChildren(std::istream &is,
                      PersistentProcessor &persistentProcessor,
                      VisibleDistantProcessor &visibleDistantProcessor,
                      TemporaryProcessor &temporaryProcessor) {
  using namespace record;

  // Expect a cell children group, though there exist empty cells,
  // like Hackdirt, so this is optional.
  if (peekGroupType(is) != Group::GroupType::CellChildren) return;

  Group cellChildren;
  is >> cellChildren;

  if (peekGroupType(is) == Group::GroupType::CellPersistentChildren) {
    Group persistentChildren;
    is >> persistentChildren;

    parseCellChildrenBlock(is, persistentProcessor);
  }

  if (peekGroupType(is) == Group::GroupType::CellVisibleDistantChildren) {
    Group visibleDistantChildren;
    is >> visibleDistantChildren;

    parseCellChildrenBlock(is, visibleDistantProcessor);
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

    parseCellChildrenBlock(is, temporaryProcessor);
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
