#include "esp_coordinator.hpp"
#include "game_settings.hpp"
#include "initial_record_visitor.hpp"
#include "record/group.hpp"
#include "record/records.hpp"
#include "resolvers/acti_resolver.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/light_resolver.hpp"
#include "resolvers/npc_resolver.hpp"
#include "resolvers/static_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include <cctype>

namespace oo {

InitialRecordVisitor::InitialRecordVisitor(oo::BaseResolversRef resolvers)
    : resolvers{std::move(resolvers)} {}

template<>
void InitialRecordVisitor::readRecord<record::ACTI>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::ACTI>().value};
  oo::getResolver<record::ACTI>(resolvers)
      .insertOrAssignEspRecord(oo::BaseId{rec.mFormId}, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::STAT>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::STAT>().value};
  oo::getResolver<record::STAT>(resolvers)
      .insertOrAssignEspRecord(oo::BaseId{rec.mFormId}, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::DOOR>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::DOOR>().value};
  oo::getResolver<record::DOOR>(resolvers)
      .insertOrAssignEspRecord(oo::BaseId{rec.mFormId}, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::LIGH>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::LIGH>().value};
  oo::getResolver<record::LIGH>(resolvers)
      .insertOrAssignEspRecord(oo::BaseId{rec.mFormId}, rec);
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
  oo::getResolver<record::STAT>(resolvers)
      .insertOrAssignEspRecord(oo::BaseId{rec.mFormId}, statRec);
}

template<>
void InitialRecordVisitor::readRecord<record::CELL>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::CELL>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::CELL>(resolvers)
      .insertOrAppend(baseId, rec, accessor);
  // Children will be loaded later, so if this cell has any then skip over them
  if (accessor.peekGroupType() == record::Group::GroupType::CellChildren) {
    accessor.skipGroup();
  }
}

template<>
void InitialRecordVisitor::readRecord<record::WRLD>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::WRLD>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::WRLD>(resolvers)
      .insertOrAppend(baseId, rec, accessor);
  // Children will be loaded later, so if this world has any then skip over them
  if (accessor.peekGroupType() == record::Group::GroupType::WorldChildren) {
    accessor.skipGroup();
  }
}

template<>
void InitialRecordVisitor::readRecord<record::NPC_>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::NPC_>().value};
  oo::getResolver<record::NPC_>(resolvers)
      .insertOrAssignEspRecord(oo::BaseId{rec.mFormId}, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::GMST>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::GMST>().value};
  GameSettings::getSingleton().load(rec, true);
}

template<>
void InitialRecordVisitor::readRecord<record::RACE>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::RACE>().value};
  oo::getResolver<record::RACE>(resolvers)
      .insertOrAssignEspRecord(oo::BaseId{rec.mFormId}, rec);
}

} // namespace oo
