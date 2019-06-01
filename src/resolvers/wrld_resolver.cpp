#include "esp/esp.hpp"
#include "math/conversions.hpp"
#include "resolvers/wrld_resolver.hpp"
#include "util/settings.hpp"
#include "wrld_impl.hpp"
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

namespace oo {

//===----------------------------------------------------------------------===//
// WRLD resolver definitions
//===----------------------------------------------------------------------===//

Resolver<record::WRLD, oo::BaseId>::Resolver(Resolver &&other) noexcept {
  std::scoped_lock lock{other.mMtx};
  mRecords = std::move(other.mRecords);
}

WrldResolver &
Resolver<record::WRLD, oo::BaseId>::operator=(Resolver &&other) noexcept {
  if (this != &other) {
    std::scoped_lock lock{mMtx, other.mMtx};
    using std::swap;
    swap(mRecords, other.mRecords);
  }

  return *this;
}

std::pair<WrldResolver::RecordIterator, bool>
WrldResolver::insertOrAppend(oo::BaseId baseId,
                             const record::WRLD &rec,
                             oo::EspAccessor accessor) {
  std::scoped_lock lock{mMtx};

  RecordEntry entry{std::make_pair(rec, tl::nullopt)};
  Metadata meta{{accessor}, {}, oo::CellGrid{}, {}};
  auto[it, inserted]{mRecords.try_emplace(baseId, entry, meta)};
  if (inserted) return {it, inserted};

  auto &wrappedEntry{it->second};
  wrappedEntry.second.mAccessors.push_back(accessor);
  wrappedEntry.first = std::make_pair(rec, tl::nullopt);

  return {it, inserted};
}

tl::optional<const record::WRLD &> WrldResolver::get(oo::BaseId baseId) const {
  std::scoped_lock lock{mMtx};
  const auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  const auto &entry{it->second.first};

  return entry.second ? *entry.second : entry.first;
}

/// \overload get(oo::BaseId)
tl::optional<record::WRLD &> WrldResolver::get(oo::BaseId baseId) {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  auto &entry{it->second.first};

  if (!entry.second) entry.second.emplace(entry.first);
  return *entry.second;
}

/// Check if there is a world with the baseId.
bool WrldResolver::contains(oo::BaseId baseId) const {
  std::scoped_lock lock{mMtx};
  return mRecords.find(baseId) != mRecords.end();
}

void WrldResolver::load(oo::BaseId baseId, BaseResolverContext baseCtx) {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return;
  auto &meta{it->second.second};
  meta.mCells.clear();
  meta.mPersistentReferences.clear();

  // Get the actual WRLD record
  auto &entry{it->second.first};
  const record::WRLD &rec{entry.second ? *entry.second : entry.first};

  // Worldspace bounds, in units.
  const auto[x0, y0]{rec.bottomLeft.data};
  const auto[x1, y1]{rec.topRight.data};

  // Worldspace bounds, in cells.
  const auto p0{oo::getCellIndex(x0, y0)};
  const auto p1{oo::getCellIndex(x1, y1)};

  meta.mCellGrid.resize(boost::extents[qvm::X(p1 - p0)][qvm::Y(p1 - p0)]);
  meta.mCellGrid.reindex(std::array{qvm::X(p0), qvm::Y(p0)});

  WrldVisitor visitor(meta, baseCtx);
  // Taking accessors by value so subsequent reads will work
  for (auto accessor : meta.mAccessors) {
    oo::readWrldChildren(accessor, oo::SkipGroupVisitorTag, visitor);
  }
}

tl::optional<oo::BaseId>
WrldResolver::getCell(oo::BaseId baseId, oo::CellIndex index) const {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  const auto &grid{it->second.second.mCellGrid};
  const auto x{qvm::X(index)}, y{qvm::Y(index)};

  // Multiarray has no builtin way to do bounds checking :(
  // By default it asserts on out-of-bounds, not throws, so we can't even 'try'.
  if (x < grid.index_bases()[0]
      || x > grid.index_bases()[0] + static_cast<int>(grid.shape()[0])
      || y < grid.index_bases()[1]
      || y > grid.index_bases()[1] + static_cast<int>(grid.shape()[1])) {
    return tl::nullopt;
  }

  return grid[x][y];
}

oo::CellGridView
WrldResolver::getNeighbourhood(oo::BaseId wrldId,
                               CellIndex cell,
                               int diameter) const {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(wrldId)};
  // We are allowed to UB if the set of cells is empty, so this is fine.
  const auto &grid{it->second.second.mCellGrid};

  const int x{qvm::X(cell)}, y{qvm::Y(cell)};
  // Adding 1 maps intervals (a, b] -> [a, b)

  // For integer i and real r,
  // floor(i - r) = i - ceil(r)
  const int x0{1 + x - diameter / 2 - (diameter % 2 == 0 ? 0 : 1)};
  const int y0{1 + y - diameter / 2 - (diameter % 2 == 0 ? 0 : 1)};
  // floor(i + r) = i + floor(r)
  const int x1{1 + x + diameter / 2};
  const int y1{1 + y + diameter / 2};

  // Keep within bounds.
  const int X0{std::max<int>(x0, grid.index_bases()[0])};
  const int Y0{std::max<int>(y0, grid.index_bases()[1])};
  const int X1{std::min<int>(x1, grid.index_bases()[0] + grid.shape()[0])};
  const int Y1{std::min<int>(y1, grid.index_bases()[1] + grid.shape()[1])};

  using Range = CellGrid::index_range;
  return grid[boost::indices[Range(X0, X1)][Range(Y0, Y1)]];
}

tl::optional<const std::unordered_set<oo::BaseId> &>
WrldResolver::getCells(oo::BaseId baseId) const {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  const auto &cells{it->second.second.mCells};
  if (cells.empty()) return tl::nullopt;
  return cells;
}

std::unordered_set<oo::BaseId> WrldResolver::getWorlds() const {
  std::scoped_lock lock{mMtx};
  std::unordered_set<oo::BaseId> ids{};
  for (const auto &[id, _] : mRecords) ids.emplace(id);
  return ids;
}

template<> void
WrldResolver::WrldVisitor::readRecord<record::CELL>(oo::EspAccessor &accessor) {
  auto &cellRes{oo::getResolver<record::CELL>(mBaseCtx)};
  const auto rec{accessor.readRecord<record::CELL>().value};
  const oo::BaseId baseId{rec.mFormId};
  cellRes.insertOrAppend(baseId, rec, accessor, /*isExterior=*/true);

  mMeta.mCells.emplace(baseId);
  const auto gridOpt{rec.grid};
  if (gridOpt) {
    oo::CellIndex p{gridOpt->data.x, gridOpt->data.y};
    mMeta.mCellGrid[qvm::X(p)][qvm::Y(p)] = baseId;
  }

  if (accessor.peekGroupType() == record::Group::GroupType::CellChildren) {
    accessor.skipGroup();
  }
}

//===----------------------------------------------------------------------===//
// World definitions
//===----------------------------------------------------------------------===//

oo::BaseId World::getBaseId() const {
  return mImpl->getBaseId();
}

std::string World::getName() const {
  return mImpl->getName();
}

void World::setName(std::string name) {
  mImpl->setName(std::move(name));
}

gsl::not_null<Ogre::SceneManager *> World::getSceneManager() const {
  return mImpl->getSceneManager();
}

gsl::not_null<World::PhysicsWorld *> World::getPhysicsWorld() const {
  return mImpl->getPhysicsWorld();
}

World::World(oo::BaseId baseId, std::string name, Resolvers resolvers)
    : mImpl(std::make_unique<WorldImpl>(baseId,
                                        std::move(name),
                                        std::move(resolvers))) {}

World::~World() = default;

void World::loadTerrain(oo::ExteriorCell &cell) {
  mImpl->loadTerrain(cell);
}

void World::loadTerrainOnly(oo::BaseId cellId, bool async) {
  mImpl->loadTerrain(cellId, async);
}

void World::unloadTerrain(oo::ExteriorCell &cell) {
  mImpl->unloadTerrain(cell);
}

void World::unloadTerrain(oo::BaseId cellId) {
  mImpl->unloadTerrain(cellId);
}

void World::updateAtmosphere(const oo::chrono::minutes &time) {
  mImpl->updateAtmosphere(time);
}

auto ReifyRecordImpl<record::WRLD>::operator()(
    const record::WRLD &refRec,
    Ogre::SceneManager *,
    btDiscreteDynamicsWorld *,
    resolvers res,
    Ogre::SceneNode *) -> type {
  const oo::BaseId baseId{refRec.mFormId};
  std::string name{refRec.name ? refRec.name->data : ""};

  auto world{std::make_shared<oo::World>(baseId, name, res)};

  return world;
}

ReifyRecordImpl<record::WRLD>::type
reifyRecord(const record::WRLD &refRec,
            ReifyRecordImpl<record::WRLD>::resolvers res) {
  return oo::reifyRecord(refRec, nullptr, nullptr, res, nullptr);
}

} // namespace oo
