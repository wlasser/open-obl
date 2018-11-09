#include "conversions.hpp"
#include "esp_coordinator.hpp"
#include "fs/path.hpp"
#include "game_settings.hpp"
#include "initial_record_visitor.hpp"
#include "record/group.hpp"
#include "record/io.hpp"
#include "records.hpp"
#include <cctype>

template<>
void InitialRecordVisitor::readRecord<record::STAT>(esp::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::STAT>().value};
  staticRes->add(BaseId{rec.id}, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::DOOR>(esp::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::DOOR>().value};
  doorRes->add(BaseId{rec.id}, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::LIGH>(esp::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::LIGH>().value};
  lightRes->add(BaseId{rec.id}, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::MISC>(esp::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::MISC>().value};
  record::raw::STAT rawStat{rec.data.editorID,
                            rec.data.modelFilename.value_or(record::MODL{}),
                            rec.data.boundRadius.value_or(record::MODB{}),
                            rec.data.textureHash};
  record::STAT statRec(rawStat, rec.flags, rec.id, rec.versionControlInfo);
  staticRes->add(BaseId{rec.id}, statRec);
}

template<>
void InitialRecordVisitor::readRecord<record::CELL>(esp::EspAccessor &accessor) {
  // Constructor takes accessor by *value* so it can be stored for deferred
  // loading. It therefore calls readRecord on the *copy*, so does not advance
  // our accessor.
  Resolver<record::CELL>::store_t entry(accessor);
  accessor.skipRecord();
  const BaseId baseId{entry.mRecord->id};
  interiorCellRes->add(baseId, std::move(entry));
  // Children will be loaded later, so if this cell has any then skip over them
  if (accessor.peekGroupType() == record::Group::GroupType::CellChildren) {
    accessor.skipGroup();
  }
}

template<>
void InitialRecordVisitor::readRecord<record::GMST>(esp::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::GMST>().value};
  GameSettings::getSingleton().load(rec, true);
}
