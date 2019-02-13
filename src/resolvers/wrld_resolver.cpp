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

oo::BaseId oo::World::getBaseId() const {
  return mBaseId;
}

std::string oo::World::getName() const {
  return mName;
}

void oo::World::setName(std::string name) {
  mName = std::move(name);
}

gsl::not_null<Ogre::SceneManager *> oo::World::getSceneManager() const {
  return mScnMgr;
}

oo::World::CellIndex oo::World::getCellIndex(float x, float y) const {
  return {
      static_cast<int32_t>(std::floor(x / oo::unitsPerCell<float>)),
      static_cast<int32_t>(std::floor(y / oo::unitsPerCell<float>))
  };
}

oo::World::World(oo::BaseId baseId, std::string name, Resolvers resolvers)
    : mBaseId(baseId), mName(std::move(name)),
      mScnMgr(Ogre::Root::getSingleton().createSceneManager()),
      mResolvers(std::move(resolvers)) {
  makeCellGrid();
}

oo::World::~World() {
  auto root{Ogre::Root::getSingletonPtr()};
  if (root) root->destroySceneManager(mScnMgr);
}

void oo::World::makeCellGrid() {
  const auto &wrldRes{oo::getResolver<record::WRLD>(mResolvers)};
  const record::WRLD &rec{*wrldRes.get(mBaseId)};

  // Worldspace bounds, in units.
  const auto[x0, y0]{rec.bottomLeft.data};
  const auto[x1, y1]{rec.topRight.data};

  // Worldspace bounds, in cells.
  const auto &p0{getCellIndex(x0, y0)};
  const auto &p1{getCellIndex(x1, y1)};

  mCells.resize(boost::extents[qvm::X(p1 - p0) + 1u][qvm::Y(p1 - p0) + 1u]);
  mCells.reindex(std::array{qvm::X(p0), qvm::Y(p0)});

  const auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};
  for (auto cellId : *wrldRes.getCells(mBaseId)) {
    const auto cellOpt{cellRes.get(cellId)};
    if (!cellOpt) continue;

    const auto gridOpt{cellOpt->grid};
    if (!gridOpt) continue;

    const auto grid{gridOpt->data};
    CellIndex p{grid.x, grid.y};
    mCells[qvm::X(p)][qvm::Y(p)] = cellId;
  }
}

oo::BaseId oo::World::getCell(CellIndex index) const {
  return mCells[qvm::X(index)][qvm::Y(index)];
}

oo::ReifyRecordTrait<record::WRLD>::type
oo::reifyRecord(const record::WRLD &refRec,
                ReifyRecordTrait<record::WRLD>::resolvers resolvers) {
  const auto &wrldRes{oo::getResolver<record::WRLD>(resolvers)};

  const oo::BaseId baseId{refRec.mFormId};
  std::string name{refRec.name ? refRec.name->data : ""};

  auto world{std::make_shared<oo::World>(baseId, name, resolvers)};

  return world;
}
