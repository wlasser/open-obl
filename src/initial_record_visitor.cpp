#include "esp/esp.hpp"
#include "esp/esp_coordinator.hpp"
#include "config/game_settings.hpp"
#include "config/globals.hpp"
#include "initial_record_visitor.hpp"
#include "record/group.hpp"
#include "record/records.hpp"
#include "resolvers/acti_resolver.hpp"
#include "resolvers/cont_resolver.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/flor_resolver.hpp"
#include "resolvers/furn_resolver.hpp"
#include "resolvers/ligh_resolver.hpp"
#include "resolvers/misc_resolver.hpp"
#include "resolvers/npc__resolver.hpp"
#include "resolvers/stat_resolver.hpp"
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

template<class F>
class PersistentChildrenVisitor {
 private:
  oo::BaseResolversRef mBaseCtx;
  oo::RefrResolversRef mRefrCtx;
  F mRefAction;

  void readRecordRefr(oo::EspAccessor &accessor);
  void readRecordAchr(oo::EspAccessor &accessor);
  // TODO: void readRecordAcre(oo::EspAccessor &accessor);
 public:
  explicit PersistentChildrenVisitor(oo::BaseResolversRef baseCtx,
                                     oo::RefrResolversRef refrCtx,
                                     F refAction) noexcept
      : mBaseCtx(std::move(baseCtx)), mRefrCtx(std::move(refrCtx)),
        mRefAction(std::forward<F>(refAction)) {}

  template<class R> void readRecord(oo::EspAccessor &accessor) {
    if constexpr (std::is_same_v<R, record::REFR>) readRecordRefr(accessor);
    else if (std::is_same_v<R, record::ACHR>) readRecordAchr(accessor);
    else accessor.skipRecord();
  }
};

template<class F> void
PersistentChildrenVisitor<F>::readRecordRefr(oo::EspAccessor &accessor) {
  const oo::BaseId baseId{accessor.peekBaseId()};

  const auto &actiRes{oo::getResolver<record::ACTI>(mBaseCtx)};
  const auto &contRes{oo::getResolver<record::CONT>(mBaseCtx)};
  const auto &doorRes{oo::getResolver<record::DOOR>(mBaseCtx)};
  const auto &lighRes{oo::getResolver<record::LIGH>(mBaseCtx)};
  const auto &miscRes{oo::getResolver<record::MISC>(mBaseCtx)};
  const auto &statRes{oo::getResolver<record::STAT>(mBaseCtx)};
  const auto &florRes{oo::getResolver<record::FLOR>(mBaseCtx)};
  const auto &furnRes{oo::getResolver<record::FURN>(mBaseCtx)};

  auto &refrActiRes{oo::getRefrResolver<record::REFR_ACTI>(mRefrCtx)};
  auto &refrContRes{oo::getRefrResolver<record::REFR_CONT>(mRefrCtx)};
  auto &refrDoorRes{oo::getRefrResolver<record::REFR_DOOR>(mRefrCtx)};
  auto &refrLighRes{oo::getRefrResolver<record::REFR_LIGH>(mRefrCtx)};
  auto &refrMiscRes{oo::getRefrResolver<record::REFR_MISC>(mRefrCtx)};
  auto &refrStatRes{oo::getRefrResolver<record::REFR_STAT>(mRefrCtx)};
  auto &refrFlorRes{oo::getRefrResolver<record::REFR_FLOR>(mRefrCtx)};
  auto &refrFurnRes{oo::getRefrResolver<record::REFR_FURN>(mRefrCtx)};

  if (actiRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_ACTI>().value};
    refrActiRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefAction(ref);
  } else if (contRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_CONT>().value};
    refrContRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefAction(ref);
  } else if (doorRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_DOOR>().value};
    refrDoorRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefAction(ref);
  } else if (lighRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_LIGH>().value};
    refrLighRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefAction(ref);
  } else if (miscRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_MISC>().value};
    refrMiscRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefAction(ref);
  } else if (statRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_STAT>().value};
    refrStatRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefAction(ref);
  } else if (florRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_FLOR>().value};
    refrFlorRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefAction(ref);
  } else if (furnRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_FURN>().value};
    refrFurnRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefAction(ref);
  } else {
    accessor.skipRecord();
  }
}

template<class F> void
PersistentChildrenVisitor<F>::readRecordAchr(oo::EspAccessor &accessor) {
  const oo::BaseId baseId{accessor.peekBaseId()};

  const auto &npc_Res{oo::getResolver<record::NPC_>(mBaseCtx)};
  auto &refrNpc_Res{oo::getRefrResolver<record::REFR_NPC_>(mRefrCtx)};

  if (npc_Res.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_NPC_>().value};
    refrNpc_Res.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mRefAction(ref);
  } else {
    accessor.skipRecord();
  }
}

class InitialWrldVisitor {
 private:
  oo::BaseResolversRef mBaseCtx;
  oo::RefrResolversRef mRefrCtx;
  oo::PersistentReferenceLocator &mRefMap;
  oo::BaseId mWrldId;

 public:
  explicit InitialWrldVisitor(oo::BaseResolversRef baseCtx,
                              oo::RefrResolversRef refrCtx,
                              oo::PersistentReferenceLocator &refMap,
                              oo::BaseId wrldId) noexcept
      : mBaseCtx(std::move(baseCtx)), mRefrCtx(std::move(refrCtx)),
        mRefMap(refMap), mWrldId(wrldId) {}

  template<class R> void readRecord(oo::EspAccessor &accessor) = delete;
};

// CWG 727
// TODO: template<> void readRecord<record::ROAD>(oo::EspAccessor &accessor);
template<> void
InitialWrldVisitor::readRecord<record::CELL>(oo::EspAccessor &accessor) {
  // Only reading a dummy cell so we can skip the actual record.
  (void) accessor.skipRecord();
  PersistentChildrenVisitor visitor(mBaseCtx, mRefrCtx, [&](const auto &ref) {
    auto posRot{ref.positionRotation};
    auto index{oo::getCellIndex(posRot.data.x, posRot.data.y)};
    mRefMap.insert(oo::RefId{ref.mFormId}, mWrldId, index);
  });

  oo::readCellChildren(accessor, visitor,
                       oo::SkipGroupVisitorTag,
                       oo::SkipGroupVisitorTag);
}

} // namespace

InitialRecordVisitor::InitialRecordVisitor(oo::BaseResolversRef baseCtx,
                                           oo::RefrResolversRef refrCtx,
                                           oo::PersistentReferenceLocator &refMap) noexcept
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
void InitialRecordVisitor::readRecord<record::CONT>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::CONT>(mBaseCtx, accessor);
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
void InitialRecordVisitor::readRecord<record::FLOR>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::FLOR>(mBaseCtx, accessor);
}

template<>
void InitialRecordVisitor::readRecord<record::FURN>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::FURN>(mBaseCtx, accessor);
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
  PersistentChildrenVisitor visitor(mBaseCtx, mRefrCtx, [&](const auto &ref) {
    mRefMap.insert(oo::RefId{ref.mFormId}, baseId);
  });
  oo::readCellChildren(accessor, visitor,
                       oo::SkipGroupVisitorTag,
                       oo::SkipGroupVisitorTag);
}

template<>
void InitialRecordVisitor::readRecord<record::WRLD>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::WRLD>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::WRLD>(mBaseCtx).insertOrAppend(baseId, rec, accessor);
  InitialWrldVisitor wrldVisitor(mBaseCtx, mRefrCtx, mRefMap, baseId);
  // All persistent references are in a dummy cell at the start of the
  // worldspace, so we can skip the inner cells.
  oo::readWrldChildren(accessor, wrldVisitor, oo::SkipGroupVisitorTag);
}

template<>
void InitialRecordVisitor::readRecord<record::WATR>(oo::EspAccessor &accessor) {
  oo::readRecordDefault<record::WATR>(mBaseCtx, accessor);
}

} // namespace oo
