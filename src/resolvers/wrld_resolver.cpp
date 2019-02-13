#include "esp.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include "settings.hpp"
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

std::pair<oo::Resolver<record::WRLD>::RecordIterator, bool>
oo::Resolver<record::WRLD>::insertOrAppend(oo::BaseId baseId,
                                           const record::WRLD &rec,
                                           oo::EspAccessor accessor) {
  spdlog::get(oo::LOG)->info("Inserting WRLD {}, {}", baseId,
                             rec.editorId.data);

  RecordEntry entry{std::make_pair(rec, tl::nullopt)};
  Metadata meta{{accessor}, {}};
  auto[it, inserted]{mRecords.try_emplace(baseId, entry, meta)};
  if (inserted) return {it, inserted};

  auto &wrappedEntry{it->second};
  wrappedEntry.second.mAccessors.push_back(accessor);
  wrappedEntry.first = std::make_pair(rec, tl::nullopt);

  return {it, inserted};
}

tl::optional<const record::WRLD &>
oo::Resolver<record::WRLD>::get(oo::BaseId baseId) const {
  const auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  const auto &entry{it->second.first};

  return entry.second ? *entry.second : entry.first;
}

/// \overload get(oo::BaseId)
tl::optional<record::WRLD &>
oo::Resolver<record::WRLD>::get(oo::BaseId baseId) {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  auto &entry{it->second.first};

  if (!entry.second) entry.second.emplace(entry.first);
  return *entry.second;
}

/// Check if there is a world with the baseId.
bool oo::Resolver<record::WRLD>::contains(oo::BaseId baseId) const {
  return mRecords.find(baseId) != mRecords.end();
}

void oo::Resolver<record::WRLD>::load(oo::BaseId baseId,
                                      BaseResolverContext baseCtx) {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return;
  auto &meta{it->second.second};
  meta.mCells.clear();

  WrldVisitor visitor(meta, baseCtx);
  // Taking accessors by value so subsequent reads will work
  for (auto accessor : meta.mAccessors) {
    oo::readWrldChildren(accessor, visitor);
  }
}

tl::optional<const absl::flat_hash_set<oo::BaseId> &>
oo::Resolver<record::WRLD>::getCells(oo::BaseId baseId) const {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  return it->second.second.mCells;
}

template<> void
oo::Resolver<record::WRLD>::WrldVisitor::readRecord<record::CELL>(oo::EspAccessor &accessor) {
  auto &cellRes{oo::getResolver<record::CELL>(mBaseCtx)};
  const auto rec{accessor.readRecord<record::CELL>().value};
  const oo::BaseId baseId{rec.mFormId};
  cellRes.insertOrAppend(baseId, rec, accessor, /*isExterior=*/true);
  mMeta.mCells.emplace(baseId);
  if (accessor.peekGroupType() == record::Group::GroupType::CellChildren) {
    accessor.skipGroup();
  }
}

oo::World::~World() {
  auto root{Ogre::Root::getSingletonPtr()};
  if (root) root->destroySceneManager(scnMgr);
}

oo::ReifyRecordTrait<record::WRLD>::type
oo::reifyRecord(const record::WRLD &refRec,
                ReifyRecordTrait<record::WRLD>::resolvers resolvers) {
  const auto &wrldRes{oo::getResolver<record::WRLD>(resolvers)};

  auto world{std::make_shared<oo::World>(oo::BaseId{refRec.mFormId})};

  return world;
}
