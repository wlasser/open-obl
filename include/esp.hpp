#ifndef OPENOBLIVION_ESP_HPP
#define OPENOBLIVION_ESP_HPP

#include "esp_coordinator.hpp"
#include "io/io.hpp"
#include "records.hpp"
#include "record/group.hpp"
#include "record/io.hpp"
#include "record/rec_of.hpp"
#include <array>
#include <string>

// The functions in this file handle reading an esp (or esm) file.
// Since these files can be quite large, it is not necessarily practical to
// load the entire file into memory. Broadly speaking, the global parsing of the
// file is handled by these functions, whereas the local parsing is delegated to
// an instance of a Processor type.
//
// A Processor is required to implement a function template
// ```
// template<class T>
// void readRecord(EspAccessor esp, EspCoordinator::SeekPos pos);
// ```
// for each type `T` arising from the class template `record::Record`.
// Instantiations of this function template are invoked by the caller when a
// record of type `T` is encountered in the esp file, with `pos` pointing to
// the beginning of the record header.
//
// If a group in the esp file is being read, then `readRecord` is guaranteed to
// be invoked for every record in the group in the order that they appear,
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
void readEsp(EspCoordinator &coordinator, int modIndex, Processor &processor);

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
void readCellChildren(EspAccessor accessor,
                      PersistentProcessor &persistentProcessor,
                      VisibleDistantProcessor &visibleDistantProcessor,
                      TemporaryProcessor &temporaryProcessor);

// Read an individual subgroup of a CellChildren subgroup, namely a
// PersistentChildren, VisibleDistantChildren, or TemporaryChildren subgroup.
template<class Processor>
void parseCellChildrenBlock(EspAccessor accessor, Processor &processor);

template<class Processor>
void readEsp(EspCoordinator &coordinator, int modIndex, Processor &processor) {
  using namespace record;

  auto accessor{coordinator.makeAccessor(modIndex)};

  // First is always a TES4 record
  if (const auto recType{accessor.peekRecordType()}; recType != "TES4") {
    throw record::RecordNotFoundError("TES4", recType);
  }
  processor.template readRecord<record::TES4>(accessor);

  // Now we expect a collection of top groups
  while (accessor.peekRecordType() == "GRUP") {
    const Group topGrp{accessor.readGroup().value};

    if (topGrp.groupType != record::Group::GroupType::Top) {
      throw record::GroupError("Expected TOP GRUP at top level");
    }

    // Get the record type of the top group. This is a little verbose because
    // we need an int to switch over.
    const auto &rt = topGrp.label.recordType;
    std::array<char, 4> recTypeArr = {rt[0], rt[1], rt[2], rt[3]};
    auto recType = recOf(recTypeArr);

    // All top groups except CELL, WRLD, and DIAL contain only records of the
    // same type as the group.
    switch (recType) {
      // Invalid record
      case 0: throw RecordNotFoundError("a record", rt);
      case "CELL"_rec: {
        using GroupType = Group::GroupType;
        // Expect a series of InteriorCellBlock groups
        while (accessor.peekGroupType() == GroupType::InteriorCellBlock) {
          const Group interiorCellBlock{accessor.readGroup().value};

          // Expect a series of InteriorCellSubblock groups
          while (accessor.peekGroupType() == GroupType::InteriorCellSubblock) {
            const Group interiorCellSubblock{accessor.readGroup().value};

            // Expect a series of cells
            while (accessor.peekRecordType() == "CELL") {
              processor.template readRecord<record::CELL>(accessor);
            }
          }
        }
        break;
      }
      case "WRLD"_rec: accessor.skipGroup();
        break;
      case "DIAL"_rec: accessor.skipGroup();
        break;
      default: {
        // Otherwise we expect a block of records all of the same type
        while (accessor.peekRecordType() == topGrp.label.recordType) {
          switch (recType) {
            case "GMST"_rec:
              processor.template readRecord<record::GMST>(accessor);
              break;
            case "GLOB"_rec:
              processor.template readRecord<record::GLOB>(accessor);
              break;
            case "CLAS"_rec:
              processor.template readRecord<record::CLAS>(accessor);
              break;
            case "FACT"_rec:
              processor.template readRecord<record::FACT>(accessor);
              break;
            case "HAIR"_rec:
              processor.template readRecord<record::HAIR>(accessor);
              break;
            case "EYES"_rec:
              processor.template readRecord<record::EYES>(accessor);
              break;
            case "RACE"_rec:
              processor.template readRecord<record::RACE>(accessor);
              break;
            case "SOUN"_rec:
              processor.template readRecord<record::SOUN>(accessor);
              break;
            case "SKIL"_rec:
              processor.template readRecord<record::SKIL>(accessor);
              break;
            case "MGEF"_rec:
              processor.template readRecord<record::MGEF>(accessor);
              break;
            case "LTEX"_rec:
              processor.template readRecord<record::LTEX>(accessor);
              break;
            case "ENCH"_rec:
              processor.template readRecord<record::ENCH>(accessor);
              break;
            case "SPEL"_rec:
              processor.template readRecord<record::SPEL>(accessor);
              break;
            case "BSGN"_rec:
              processor.template readRecord<record::BSGN>(accessor);
              break;
            case "DOOR"_rec:
              processor.template readRecord<record::DOOR>(accessor);
              break;
            case "LIGH"_rec:
              processor.template readRecord<record::LIGH>(accessor);
              break;
            case "MISC"_rec:
              processor.template readRecord<record::MISC>(accessor);
              break;
            case "STAT"_rec:
              processor.template readRecord<record::STAT>(accessor);
              break;
            case "ALCH"_rec:
              processor.template readRecord<record::ALCH>(accessor);
              break;
            default: accessor.skipRecord();
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
void readCellChildren(EspAccessor accessor,
                      PersistentProcessor &persistentProcessor,
                      VisibleDistantProcessor &visibleDistantProcessor,
                      TemporaryProcessor &temporaryProcessor) {
  using namespace record;
  using GroupType = Group::GroupType;

  // Expect a cell children group, though there exist empty cells,
  // like Hackdirt, so this is optional.
  if (accessor.peekGroupType() != GroupType::CellChildren) {
    return;
  }

  const Group cellChildren{accessor.readGroup().value};

  if (accessor.peekGroupType() == GroupType::CellPersistentChildren) {
    const Group persistentChildren{accessor.readGroup().value};
    parseCellChildrenBlock(accessor, persistentProcessor);
  }

  if (accessor.peekGroupType() == GroupType::CellVisibleDistantChildren) {
    const Group visibleDistantChildren{accessor.readGroup().value};
    parseCellChildrenBlock(accessor, visibleDistantProcessor);
  }

  if (accessor.peekGroupType() == GroupType::CellTemporaryChildren) {
    const Group temporaryChildren{accessor.readGroup().value};

    // Unsure if PGRD is usually optional or not, but sometimes this entire
    // group is empty e.g. ImperialSewerSystemTG11
    if (accessor.peekRecordType() == "PGRD") {
      // TODO: PGRD
      accessor.skipRecord();
    }

    parseCellChildrenBlock(accessor, temporaryProcessor);
  }
}

template<class Processor>
void parseCellChildrenBlock(EspAccessor accessor, Processor &processor) {
  using namespace record;
  for (;;) {
    const auto type{accessor.peekRecordType()};
    if (type == "REFR") {
      processor.template readRecord<record::REFR>(accessor);
    } else if (type == "ACHR") {
      accessor.skipRecord();
    } else if (type == "ACRE") {
      accessor.skipRecord();
    } else {
      return;
    }
  }
}

} // namespace esp

#endif // OPENOBLIVION_ESP_HPP
