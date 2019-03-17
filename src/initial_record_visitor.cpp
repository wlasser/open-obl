#include "esp_coordinator.hpp"
#include "game_settings.hpp"
#include "globals.hpp"
#include "initial_record_visitor.hpp"
#include "record/group.hpp"
#include "record/records.hpp"
#include "resolvers/acti_resolver.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/light_resolver.hpp"
#include "resolvers/misc_resolver.hpp"
#include "resolvers/npc_resolver.hpp"
#include "resolvers/static_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include <cctype>

namespace oo {

InitialRecordVisitor::InitialRecordVisitor(oo::BaseResolversRef resolvers)
    : resolvers{std::move(resolvers)} {}

template<>
void InitialRecordVisitor::readRecord<record::GMST>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::GMST>().value};
  GameSettings::getSingleton().load(rec, true);
}

template<>
void InitialRecordVisitor::readRecord<record::GLOB>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::GLOB>().value};
  Globals::getSingleton().load(rec, true);
}

template<>
void InitialRecordVisitor::readRecord<record::RACE>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::RACE>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::RACE>(mBaseCtx).insertOrAssignEspRecord(baseId, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::LTEX>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::LTEX>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::LTEX>(mBaseCtx).insertOrAssignEspRecord(baseId, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::ACTI>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::ACTI>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::ACTI>(mBaseCtx).insertOrAssignEspRecord(baseId, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::DOOR>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::DOOR>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::DOOR>(mBaseCtx).insertOrAssignEspRecord(baseId, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::LIGH>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::LIGH>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::LIGH>(mBaseCtx).insertOrAssignEspRecord(baseId, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::MISC>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::MISC>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::MISC>(mBaseCtx).insertOrAssignEspRecord(baseId, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::STAT>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::STAT>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::STAT>(mBaseCtx).insertOrAssignEspRecord(baseId, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::GRAS>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::GRAS>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::GRAS>(mBaseCtx).insertOrAssignEspRecord(baseId, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::TREE>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::TREE>().value};

  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::TREE>(mBaseCtx).insertOrAssignEspRecord(baseId, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::NPC_>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::NPC_>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::NPC_>(mBaseCtx).insertOrAssignEspRecord(baseId, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::WTHR>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::WTHR>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::WTHR>(mBaseCtx).insertOrAssignEspRecord(baseId, rec);
}

template<>
void InitialRecordVisitor::readRecord<record::CLMT>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::CLMT>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::CLMT>(mBaseCtx).insertOrAssignEspRecord(baseId, rec);
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
  oo::getResolver<record::WRLD>(mBaseCtx).insertOrAppend(baseId, rec, accessor);
  // Children will be loaded later, so if this world has any then skip over them
  if (accessor.peekGroupType() == record::Group::GroupType::WorldChildren) {
    accessor.skipGroup();
  }
}

template<>
void InitialRecordVisitor::readRecord<record::WATR>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::WATR>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::WATR>(mBaseCtx).insertOrAssignEspRecord(baseId, rec);
}

} // namespace oo
