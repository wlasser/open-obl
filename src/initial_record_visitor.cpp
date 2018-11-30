#include "conversions.hpp"
#include "esp_coordinator.hpp"
#include "fs/path.hpp"
#include "game_settings.hpp"
#include "initial_record_visitor.hpp"
#include "record/group.hpp"
#include "record/io.hpp"
#include "record/records.hpp"
#include <cctype>

template<>
void InitialRecordVisitor::readRecord<record::ACTI>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::ACTI>().value};
  activatorRes->insertOrAssignEspRecord(BaseId{rec.mFormId}, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::STAT>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::STAT>().value};
  staticRes->insertOrAssignEspRecord(BaseId{rec.mFormId}, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::DOOR>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::DOOR>().value};
  doorRes->insertOrAssignEspRecord(BaseId{rec.mFormId}, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::LIGH>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::LIGH>().value};
  lightRes->insertOrAssignEspRecord(BaseId{rec.mFormId}, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::MISC>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::MISC>().value};
  record::raw::STAT rawStat{rec.editorId,
                            rec.modelFilename.value_or(record::MODL{}),
                            rec.boundRadius.value_or(record::MODB{}),
                            rec.textureHash};
  record::STAT statRec(rawStat,
                       rec.mRecordFlags,
                       rec.mFormId,
                       rec.mVersionControlInfo);
  staticRes->insertOrAssignEspRecord(BaseId{rec.mFormId}, statRec);
}

template<>
void InitialRecordVisitor::readRecord<record::CELL>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::CELL>().value};
  const BaseId baseId{rec.mFormId};
  cellRes->insertOrAppend(baseId, rec, accessor);
  // Children will be loaded later, so if this cell has any then skip over them
  if (accessor.peekGroupType() == record::Group::GroupType::CellChildren) {
    accessor.skipGroup();
  }
}

template<>
void InitialRecordVisitor::readRecord<record::GMST>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::GMST>().value};
  GameSettings::getSingleton().load(rec, true);
}
