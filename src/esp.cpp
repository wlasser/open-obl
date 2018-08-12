#include "esp.hpp"
#include "io/io.hpp"
#include "records.hpp"
#include "record/group.hpp"
#include "record/io.hpp"
#include "record/rec_of.hpp"
#include <array>
#include <istream>
#include <iostream>
#include <ostream>
#include <string>

Esp::Esp(std::istream &is) {
  // First is always a TES4 record
  record::peekOrThrow(is, "TES4");
  record::TES4 rec;
  is >> rec;

  // Now we expect a collection of top groups
  while (record::peekRecordType(is) == "GRUP") {
    record::Group topGrp;
    is >> topGrp;

    if (topGrp.groupType != record::Group::GroupType::Top) {
      throw record::GroupError("Expected TOP GRUP at top level");
    }

    // Get the record type of the top group. This is a little verbose
    // because we need an int to switch over.
    using namespace record;
    const auto &rt = topGrp.label.recordType;
    std::clog << rt << '\n';
    std::array<char, 4> recTypeArr = {rt[0], rt[1], rt[2], rt[3]};
    auto recType = recOf(recTypeArr);

    // All top groups except CELL, WRLD, and DIAL contain only records of the
    // same type as the group.
    switch (recType) {
      // Invalid record
      case 0: {
        // Throw something
        break;
      }
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
              parseCell(is);
            }
          }
        }
        //skipRecord(is);
        break;
      }
      case "WRLD"_rec: skipRecord(is);
        break;
      case "DIAL"_rec: skipRecord(is);
        break;
      default: {
        // Otherwise we expect a block of records all of the same type
        while (peekRecordType(is) == topGrp.label.recordType) {
          switch (recType) {
            case "GMST"_rec: parseRecord<record::GMST>(is);
              break;
            case "GLOB"_rec: parseRecord<record::GLOB>(is);
              break;
            case "CLAS"_rec: parseRecord<record::CLAS>(is);
              break;
            case "FACT"_rec: parseRecord<record::FACT>(is);
              break;
            case "HAIR"_rec: parseRecord<record::HAIR>(is);
              break;
            case "EYES"_rec: parseRecord<record::EYES>(is);
              break;
            case "RACE"_rec: parseRecord<record::RACE>(is);
              break;
            case "SOUN"_rec: parseRecord<record::SOUN>(is);
              break;
            case "SKIL"_rec: parseRecord<record::SKIL>(is);
              break;
            case "MGEF"_rec: parseRecord<record::MGEF>(is);
              break;
            case "LTEX"_rec: parseRecord<record::LTEX>(is);
              break;
            case "STAT"_rec: parseRecord<record::STAT>(is);
              break;
            case "ALCH"_rec: parseRecord<record::ALCH>(is);
              break;
            default: skipRecord(is);
              break;
          }
        }
        break;
      }
    }
  }
}

void Esp::parseCell(std::istream &is) {
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
