#include "engine/initial_processor.hpp"
#include "record/group.hpp"
#include "record/io.hpp"
#include "records.hpp"
#include <cctype>

namespace engine {

template<>
void InitialProcessor::readRecord<record::STAT>(std::istream &is) {
  auto rec = record::readRecord<record::STAT>(is);
  StaticEntry entry{};
  // MODL records omit the "meshes/" folder
  entry.modelFilename = rec.data.modelFilename.data;
  // Convert from win to nix
  std::transform(rec.data.modelFilename.data.begin(),
                 rec.data.modelFilename.data.end(),
                 entry.modelFilename.begin(),
                 [](unsigned char c) -> unsigned char {
                   return static_cast<unsigned char>(std::tolower(
                       c == '\\' ? '/' : c));
                 });
  entry.modelFilename = "meshes/" + entry.modelFilename;
  staticMgr->statics[rec.id] = std::move(entry);
}

template<>
void InitialProcessor::readRecord<record::CELL>(std::istream &is) {
  InteriorCellEntry entry{};
  entry.tell = is.tellg();
  entry.record = std::make_unique<record::CELL>();
  record::readRecord(is, *entry.record, "CELL");
  interiorCellMgr->cells[entry.record->id] = std::move(entry);
  // Children will be loaded later, so if this cell has any then skip over them
  if (record::peekGroupType(is) == record::Group::GroupType::CellChildren) {
    record::skipGroup(is);
  }
}

} // namespace engine