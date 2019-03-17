#ifndef OPENOBLIVION_ESP_HPP
#define OPENOBLIVION_ESP_HPP

#include "esp_coordinator.hpp"
#include "io/io.hpp"
#include "record/group.hpp"
#include "record/io.hpp"
#include "record/rec_of.hpp"
#include "record/records.hpp"
#include <array>
#include <string>

/// \file esp.hpp
/// Functions handling the reading of esp (or esm) files.
/// Since these files can be quite large, it is not necessarily practical to
/// load the entire file into memory. Broadly speaking, the global parsing of
/// the file is handled by these functions, whereas the local parsing is
/// delegated to an instance of a `RecordVisitor` type.
///
/// A `RecordVisitor` is required to implement a function template
/// ```
/// template<class T>
/// void readRecord(EspAccessor esp);
/// ```
/// for each type `T` arising from the class template `record::Record`.
/// Instantiations of this function template are invoked by the caller when a
/// record of type `T` is encountered in the esp file, with `pos` pointing to
/// the beginning of the record header.
///
/// If a group in the esp file is being read, then `readRecord` is guaranteed to
/// be invoked for every record in the group in the order that they appear,
/// expect in the groups `record::CELL`, `record::WRLD`, and `record::DIAL`.
/// Some of the entries in these groups contain a list of child groups, which
/// the `RecordVisitor` may handle differently.
///
/// When a `record::CELL` appears, it is (almost) always followed by a
/// `CellChildren` subgroup. It is expected that `readRecord<record::CELL>`
/// read (or skip) both the `record::CELL` and all its children.
/// The `oo::readCellChildren` method assists with this.
///
/// When a `record::WRLD` appears, it is always followed by a `WorldChildren`
/// subgroup. It is expected that `readRecord<record::WRLD>` read (or skip) both
/// the `record::WRLD` and all its children. The `oo::readWrldChildren` method
/// assists with this.
namespace oo {

/// Read an entire esp file from the beginning, delegating the actual reading
/// to the visitor.
template<class RecordVisitor>
void readEsp(EspCoordinator &coordinator, int modIndex, RecordVisitor &visitor);

/// Read the `CellChildren` subgroup following a `record::CELL`. The reading of
/// the `PersistentChildren`, `VisibleDistantChildren`, and `TemporaryChildren`
/// are delegated to the corresponding visitors.
///
/// The first two visitors must be able to read `record::REFR`, `record::ACHR`,
/// and `record::ACRE`. The third visitor must be able to read `record::REFR`,
/// `record::ACHR`, `record::ACRE`, and `record::PGRD` records. If the parent
/// `record::CELL` is part of an exterior cell, then the third visitor must also
/// be able to read `record::LAND` records.
///
/// Note that in rare cases, a `record::CELL` may not have any children, in
/// which case this function does nothing.
template<class PersistentVisitor,
    class VisibleDistantVisitor,
    class TemporaryVisitor>
void readCellChildren(EspAccessor &accessor,
                      PersistentVisitor &persistentVisitor,
                      VisibleDistantVisitor &visibleDistantVisitor,
                      TemporaryVisitor &temporaryVisitor);

/// Read the `record::LAND` and `record::PGRD` children of the cell. The
/// `EspAccessor` is taken by value because the final position in the cell is
/// unpredictable.
template<class Visitor>
void readCellTerrain(EspAccessor accessor, Visitor &visitor);

/// Read an individual subgroup of a `CellChildren` subgroup, namely a
/// `PersistentChildren`, `VisibleDistantChildren`, or `TemporaryChildren`
/// subgroup.
template<class RecordVisitor>
void parseCellChildrenBlock(EspAccessor &accessor, RecordVisitor &visitor);

/// Read the `WorldChildren` subgroup following a `record::WRLD` record.
/// The reading of the outer `record::ROAD` and `record::CELL` records, as well
/// as all inner `record::CELL` records, are delegated to the `visitor`. Note
/// that `record::CELL` records are followed by children, and
/// `readRecord<record::CELL>` is expected to read the children too.
template<class Visitor>
void readWrldChildren(EspAccessor &accessor, Visitor &visitor);

struct SkipGroupVisitorTag_t {};
constexpr static inline SkipGroupVisitorTag_t SkipGroupVisitorTag{};

//===----------------------------------------------------------------------===//
// Function template definitions
//===----------------------------------------------------------------------===//

template<class RecordVisitor>
void readEsp(EspCoordinator &coordinator,
             int modIndex,
             RecordVisitor &visitor) {
  using record::operator ""_rec;

  auto accessor{coordinator.makeAccessor(modIndex)};

  // First is always a TES4 record
  if (const auto recType{accessor.peekRecordType()}; recType != "TES4"_rec) {
    throw record::RecordNotFoundError("TES4", record::recOf(recType));
  }
  visitor.template readRecord<record::TES4>(accessor);

  // Now we expect a collection of top groups
  while (accessor.peekRecordType() == "GRUP"_rec) {
    const record::Group topGrp{accessor.readGroup().value};

    if (topGrp.groupType != record::Group::GroupType::Top) {
      throw record::GroupError("Expected TOP GRUP at top level");
    }

    const auto &rt{topGrp.label.recordType};
    const uint32_t recType{record::recOf(rt)};

    // All top groups except CELL, WRLD, and DIAL contain only records of the
    // same type as the group.
    switch (recType) {
      // Invalid record
      case 0: throw record::RecordNotFoundError("a record", rt);
      case "CELL"_rec: {
        using GroupType = record::Group::GroupType;
        // Expect a series of InteriorCellBlock groups
        while (accessor.peekGroupType() == GroupType::InteriorCellBlock) {
          (void) accessor.readGroup();

          // Expect a series of InteriorCellSubblock groups
          while (accessor.peekGroupType() == GroupType::InteriorCellSubblock) {
            (void) accessor.readGroup();

            // Expect a series of cells
            while (accessor.peekRecordType() == "CELL"_rec) {
              visitor.template readRecord<record::CELL>(accessor);
            }
          }
        }
        break;
      }
      case "WRLD"_rec: {
        // Unlike CELL, `readRecord<record::WRLD>` is expected to take care of
        // the block and subblock groups using `readWrldChildren`; we are not
        // required to do anything special.
        while (accessor.peekRecordType() == "WRLD"_rec) {
          visitor.template readRecord<record::WRLD>(accessor);
        }
        break;
      }
      case "DIAL"_rec: accessor.skipGroup();
        break;
      default: {
        // Otherwise we expect a block of records all of the same type
        while (accessor.peekRecordType() == recType) {
          switch (recType) {
            case "GMST"_rec:visitor.template readRecord<record::GMST>(accessor);
              break;
            case "GLOB"_rec:visitor.template readRecord<record::GLOB>(accessor);
              break;
            case "CLAS"_rec:visitor.template readRecord<record::CLAS>(accessor);
              break;
            case "FACT"_rec:visitor.template readRecord<record::FACT>(accessor);
              break;
            case "HAIR"_rec:visitor.template readRecord<record::HAIR>(accessor);
              break;
            case "EYES"_rec:visitor.template readRecord<record::EYES>(accessor);
              break;
            case "RACE"_rec:visitor.template readRecord<record::RACE>(accessor);
              break;
            case "SOUN"_rec:visitor.template readRecord<record::SOUN>(accessor);
              break;
            case "SKIL"_rec:visitor.template readRecord<record::SKIL>(accessor);
              break;
            case "MGEF"_rec:visitor.template readRecord<record::MGEF>(accessor);
              break;
            case "LTEX"_rec:visitor.template readRecord<record::LTEX>(accessor);
              break;
            case "ENCH"_rec:visitor.template readRecord<record::ENCH>(accessor);
              break;
            case "SPEL"_rec:visitor.template readRecord<record::SPEL>(accessor);
              break;
            case "BSGN"_rec:visitor.template readRecord<record::BSGN>(accessor);
              break;
            case "ACTI"_rec:visitor.template readRecord<record::ACTI>(accessor);
              break;
            case "DOOR"_rec:visitor.template readRecord<record::DOOR>(accessor);
              break;
            case "LIGH"_rec:visitor.template readRecord<record::LIGH>(accessor);
              break;
            case "MISC"_rec:visitor.template readRecord<record::MISC>(accessor);
              break;
            case "STAT"_rec:visitor.template readRecord<record::STAT>(accessor);
              break;
            case "GRAS"_rec:visitor.template readRecord<record::GRAS>(accessor);
              break;
            case "TREE"_rec:visitor.template readRecord<record::TREE>(accessor);
              break;
            case "NPC_"_rec:visitor.template readRecord<record::NPC_>(accessor);
              break;
            case "ALCH"_rec:visitor.template readRecord<record::ALCH>(accessor);
              break;
            case "WTHR"_rec:visitor.template readRecord<record::WTHR>(accessor);
              break;
            case "CLMT"_rec:visitor.template readRecord<record::CLMT>(accessor);
              break;
            case "WATR"_rec:visitor.template readRecord<record::WATR>(accessor);
              break;
            default: accessor.skipRecord();
              break;
          }
        }
      }
    }
  }
}

template<class PersistentVisitor,
    class VisibleDistantVisitor,
    class TemporaryVisitor>
void readCellChildren(EspAccessor &accessor,
                      PersistentVisitor &persistentVisitor,
                      VisibleDistantVisitor &visibleDistantVisitor,
                      TemporaryVisitor &temporaryVisitor) {
  using namespace record;
  using GroupType = Group::GroupType;

  // Expect a cell children group, though there exist empty cells,
  // like Hackdirt, so this is optional.
  if (accessor.peekGroupType() != GroupType::CellChildren) return;
  (void) accessor.readGroup();

  if (accessor.peekGroupType() == GroupType::CellPersistentChildren) {
    if constexpr (std::is_same_v<std::decay_t<PersistentVisitor>,
                                 SkipGroupVisitorTag_t>) {
      accessor.skipGroup();
    } else {
      (void) accessor.readGroup();
      parseCellChildrenBlock(accessor, persistentVisitor);
    }
  }

  if (accessor.peekGroupType() == GroupType::CellVisibleDistantChildren) {
    if constexpr (std::is_same_v<std::decay_t<VisibleDistantVisitor>,
                                 SkipGroupVisitorTag_t>) {
      accessor.skipGroup();
    } else {
      (void) accessor.readGroup();
      parseCellChildrenBlock(accessor, visibleDistantVisitor);
    }
  }

  if (accessor.peekGroupType() == GroupType::CellTemporaryChildren) {
    if constexpr (std::is_same_v<std::decay_t<TemporaryVisitor>,
                                 SkipGroupVisitorTag_t>) {
      accessor.skipGroup();
    } else {
      (void) accessor.readGroup();

      if (accessor.peekRecordType() == "LAND"_rec) {
        temporaryVisitor.template readRecord<record::LAND>(accessor);
      }

      // Unsure if PGRD is usually optional or not, but sometimes this entire
      // group is empty e.g. ImperialSewerSystemTG11
      if (accessor.peekRecordType() == "PGRD"_rec) {
        // TODO: PGRD
        accessor.skipRecord();
      }

      parseCellChildrenBlock(accessor, temporaryVisitor);
    }
  }
}

template<class Visitor>
void readCellTerrain(EspAccessor accessor, Visitor &visitor) {
  using record::operator ""_rec;
  using GroupType = record::Group::GroupType;

  if (accessor.peekGroupType() != GroupType::CellChildren) return;
  (void) accessor.readGroup();

  if (accessor.peekGroupType() == GroupType::CellPersistentChildren) {
    accessor.skipGroup();
  }

  if (accessor.peekGroupType() == GroupType::CellVisibleDistantChildren) {
    accessor.skipGroup();
  }

  if (accessor.peekGroupType() != GroupType::CellTemporaryChildren) return;
  (void) accessor.readGroup();

  if (accessor.peekRecordType() == "LAND"_rec) {
    visitor.template readRecord<record::LAND>(accessor);
  }

  if (accessor.peekRecordType() == "PGRD"_rec) {
    // TODO: PGRD
    accessor.skipRecord();
  }
}

template<class Visitor>
void readWrldChildren(EspAccessor &accessor, Visitor &visitor) {
  using record::operator ""_rec;
  using GroupType = record::Group::GroupType;

  // Expect a world children group
  if (accessor.peekGroupType() != GroupType::WorldChildren) return;
  (void) accessor.readGroup();

  // Optional road information, only the two main worldspaces have this.
  if (accessor.peekRecordType() == "ROAD"_rec) {
    // TODO: ROAD
    accessor.skipRecord();
  }

  // Dummy cell containing all the persistent references in the entire
  // worldspace.
  if (accessor.peekRecordType() == "CELL"_rec) {
    visitor.template readRecord<record::CELL>(accessor);
  }

  // Expect a series of ExteriorCellBlock groups
  while (accessor.peekGroupType() == GroupType::ExteriorCellBlock) {
    (void) accessor.readGroup();

    // Expect a series of ExteriorCellSubblock groups
    while (accessor.peekGroupType() == GroupType::ExteriorCellSubblock) {
      (void) accessor.readGroup();

      // Expect a series of cells
      while (accessor.peekRecordType() == "CELL"_rec) {
        visitor.template readRecord<record::CELL>(accessor);
      }
    }
  }
}

template<class RecordVisitor>
void parseCellChildrenBlock(EspAccessor &accessor, RecordVisitor &visitor) {
  using namespace record;
  for (;;) {
    switch (accessor.peekRecordType()) {
      case "REFR"_rec: visitor.template readRecord<record::REFR>(accessor);
        break;
      case "ACHR"_rec: visitor.template readRecord<record::ACHR>(accessor);
        break;
      case "ACRE"_rec: accessor.skipRecord();
        break;
      default: return;
    }
  }
}

} // namespace oo

#endif // OPENOBLIVION_ESP_HPP
