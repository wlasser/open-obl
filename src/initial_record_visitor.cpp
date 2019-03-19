#include "esp.hpp"
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

namespace {

template<class R> void readRecordDefault(const oo::BaseResolversRef &baseCtx,
                                         oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<R>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<R>(baseCtx).insertOrAssignEspRecord(baseId, rec);
}

class PersistentChildrenVisitor {
 private:
  oo::BaseResolversRef mBaseCtx;
  oo::RefrResolversRef mRefrCtx;
  using PersistentRefMap = absl::flat_hash_map<oo::RefId, oo::BaseId>;
  PersistentRefMap &mRefMap;
  oo::BaseId mCellId;

 public:
  explicit PersistentChildrenVisitor(oo::BaseResolversRef baseCtx,
                                     oo::RefrResolversRef refrCtx,
                                     PersistentRefMap &refMap,
                                     oo::BaseId cellId) noexcept
      : mBaseCtx(std::move(baseCtx)), mRefrCtx(std::move(refrCtx)),
        mRefMap(refMap), mCellId(cellId) {}

  template<class R> void readRecord(oo::EspAccessor &accessor);

  template<> void readRecord<record::REFR>(oo::EspAccessor &accessor);
  template<> void readRecord<record::ACHR>(oo::EspAccessor &accessor);
  // TODO: template<> void readRecord<record::ACRE>(oo::EspAccessor &accessor);
};

template<> void
PersistentChildrenVisitor::readRecord<record::REFR>(oo::EspAccessor &accessor) {
  const oo::BaseId baseId{accessor.peekBaseId()};

  const auto &actiRes{oo::getResolver<record::ACTI>(mBaseCtx)};
  const auto &doorRes{oo::getResolver<record::DOOR>(mBaseCtx)};
  const auto &lighRes{oo::getResolver<record::LIGH>(mBaseCtx)};
  const auto &miscRes{oo::getResolver<record::MISC>(mBaseCtx)};
  const auto &statRes{oo::getResolver<record::STAT>(mBaseCtx)};

  auto &refrActiRes{oo::getRefrResolver<record::REFR_ACTI>(mRefrCtx)};
  auto &refrDoorRes{oo::getRefrResolver<record::REFR_DOOR>(mRefrCtx)};
  auto &refrLighRes{oo::getRefrResolver<record::REFR_LIGH>(mRefrCtx)};
  auto &refrMiscRes{oo::getRefrResolver<record::REFR_MISC>(mRefrCtx)};
  auto &refrStatRes{oo::getRefrResolver<record::REFR_STAT>(mRefrCtx)};

  if (actiRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_ACTI>().value};
    refrActiRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefMap[oo::RefId{ref.mFormId}] = mCellId;
  } else if (doorRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_DOOR>().value};
    refrDoorRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefMap[oo::RefId{ref.mFormId}] = mCellId;
  } else if (lighRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_LIGH>().value};
    refrLighRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefMap[oo::RefId{ref.mFormId}] = mCellId;
  } else if (miscRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_MISC>().value};
    refrMiscRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefMap[oo::RefId{ref.mFormId}] = mCellId;
  } else if (statRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_STAT>().value};
    refrStatRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefMap[oo::RefId{ref.mFormId}] = mCellId;
  } else {
    accessor.skipRecord();
  }
}

template<> void
PersistentChildrenVisitor::readRecord<record::ACHR>(oo::EspAccessor &accessor) {
  const oo::BaseId baseId{accessor.peekBaseId()};

  const auto &npc_Res{oo::getResolver<record::NPC_>(mBaseCtx)};
  auto &refrNpc_Res{oo::getRefrResolver<record::REFR_NPC_>(mRefrCtx)};

  if (npc_Res.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_NPC_>().value};
    refrNpc_Res.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefMap[oo::RefId{ref.mFormId}] = mCellId;
  } else {
    accessor.skipRecord();
  }
}

class InitialWrldVisitor {
 private:
  oo::BaseResolversRef mBaseCtx;
  oo::RefrResolversRef mRefrCtx;
  using PersistentRefMap = absl::flat_hash_map<oo::RefId, oo::BaseId>;
  PersistentRefMap &mRefMap;

 public:
  explicit InitialWrldVisitor(oo::BaseResolversRef baseCtx,
                              oo::RefrResolversRef refrCtx,
                              PersistentRefMap &refMap) noexcept
      : mBaseCtx(std::move(baseCtx)), mRefrCtx(std::move(refrCtx)),
        mRefMap(refMap) {}

  template<class R> void readRecord(oo::EspAccessor &accessor);

  template<> void readRecord<record::CELL>(oo::EspAccessor &accessor);
  // TODO: template<> void readRecord<record::ROAD>(oo::EspAccessor &accessor);
};

template<> void
InitialWrldVisitor::readRecord<record::CELL>(oo::EspAccessor &accessor) {
  const auto result{accessor.skipRecord()};
  const oo::BaseId cellId{result.header.id};
  PersistentChildrenVisitor persistentChildrenVisitor(mBaseCtx, mRefrCtx,
                                                      mRefMap, cellId);
  oo::readCellChildren(accessor, persistentChildrenVisitor,
                       oo::SkipGroupVisitorTag,
                       oo::SkipGroupVisitorTag);
}

} // namespace

InitialRecordVisitor::InitialRecordVisitor(oo::BaseResolversRef baseCtx,
                                           oo::RefrResolversRef refrCtx,
                                           PersistentRefMap &refMap) noexcept
    : mBaseCtx(std::move(baseCtx)), mRefrCtx(std::move(refrCtx)),
      mRefMap(refMap) {}

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
  oo::readRecordDefault<record::RACE>(mBaseCtx, accessor);
}

template<>
void InitialRecordVisitor::readRecord<record::LTEX>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::LTEX>(mBaseCtx, accessor);
}

template<>
void InitialRecordVisitor::readRecord<record::ACTI>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::ACTI>(mBaseCtx, accessor);
}

template<>
void InitialRecordVisitor::readRecord<record::DOOR>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::DOOR>(mBaseCtx, accessor);
}

template<>
void InitialRecordVisitor::readRecord<record::LIGH>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::LIGH>(mBaseCtx, accessor);
}

template<>
void InitialRecordVisitor::readRecord<record::MISC>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::MISC>(mBaseCtx, accessor);
}

template<>
void InitialRecordVisitor::readRecord<record::STAT>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::STAT>(mBaseCtx, accessor);
}

template<>
void InitialRecordVisitor::readRecord<record::GRAS>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::GRAS>(mBaseCtx, accessor);
}

template<>
void InitialRecordVisitor::readRecord<record::TREE>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::TREE>(mBaseCtx, accessor);
}

template<>
void InitialRecordVisitor::readRecord<record::NPC_>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::NPC_>(mBaseCtx, accessor);
}

template<>
void InitialRecordVisitor::readRecord<record::WTHR>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::WTHR>(mBaseCtx, accessor);
}

template<>
void InitialRecordVisitor::readRecord<record::CLMT>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::CLMT>(mBaseCtx, accessor);
}

template<>
void InitialRecordVisitor::readRecord<record::CELL>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::CELL>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::CELL>(mBaseCtx).insertOrAppend(baseId, rec, accessor);
  PersistentChildrenVisitor persistentChildrenVisitor(mBaseCtx, mRefrCtx,
                                                      mRefMap, baseId);
  oo::readCellChildren(accessor, persistentChildrenVisitor,
                       oo::SkipGroupVisitorTag,
                       oo::SkipGroupVisitorTag);
}

template<>
void InitialRecordVisitor::readRecord<record::WRLD>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::WRLD>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::WRLD>(mBaseCtx).insertOrAppend(baseId, rec, accessor);
  InitialWrldVisitor wrldVisitor(mBaseCtx, mRefrCtx, mRefMap);
  oo::readWrldChildren(accessor, wrldVisitor);
  // TODO: The first cell in a worldspace contains all the persistent references
  //       for that worldspace, so we can do a lot better here by only reading
  //       that dummy cell. This requires modifying readWrldChildren() or
  //       inlining part of it here.
}

template<>
void InitialRecordVisitor::readRecord<record::WATR>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::WATR>(mBaseCtx, accessor);
}

} // namespace oo
