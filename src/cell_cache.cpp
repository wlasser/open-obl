#include "cell_cache.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include <algorithm>
#include <mutex>

namespace oo {

CellCache::GetResult CellCache::getCell(oo::BaseId id) const {
  const auto isId = [id](const auto &ptr) { return ptr->getBaseId() == id; };
  std::scoped_lock lock{mMutex};

  auto intIt{std::find_if(mInteriors.begin(), mInteriors.end(), isId)};
  if (intIt != mInteriors.end()) return GetResult{*intIt, true};

  auto extIt{std::find_if(mExteriors.begin(), mExteriors.end(), isId)};
  if (extIt != mExteriors.end()) return GetResult{*extIt, false};

  return GetResult{CellPtr(), false};
}

void CellCache::push_back(const InteriorPtr &interiorCell) {
  std::scoped_lock lock{mMutex};
  mInteriors.push_back(interiorCell);
}

void CellCache::push_back(const ExteriorPtr &exteriorCell) {
  std::scoped_lock lock{mMutex};
  mExteriors.push_back(exteriorCell);

#ifndef NDEBUG
  std::string exteriorCache{" "};
  std::string counts{" "};
  for (const auto &ptr : mExteriors) {
    exteriorCache += ptr->getBaseId().string() + ' ';
    counts += std::to_string(ptr.use_count()) + "        ";
  }
  spdlog::get(oo::LOG)->trace("push_back(): Cache is   [{}]", exteriorCache);
  spdlog::get(oo::LOG)->trace("push_back(): Counts are [{}]", counts);
#endif
}

void CellCache::promoteCell(oo::BaseId id) {
  std::scoped_lock lock{mMutex};
  const auto isId = [id](const auto &ptr) { return ptr->getBaseId() == id; };

  auto intIt{std::find_if(mInteriors.begin(), mInteriors.end(), isId)};
  if (intIt != mInteriors.end() && std::next(intIt) != mInteriors.end()) {
    std::rotate(intIt, std::next(intIt), mInteriors.end());
    return;
  }

  auto extIt{std::find_if(mExteriors.begin(), mExteriors.end(), isId)};
  if (extIt != mExteriors.end() && std::next(extIt) != mExteriors.end()) {
    std::rotate(extIt, std::next(extIt), mExteriors.end());
  }

#ifndef NDEBUG
  std::string exteriorCache{" "};
  std::string counts{" "};
  for (const auto &ptr : mExteriors) {
    exteriorCache += ptr->getBaseId().string() + ' ';
    counts += std::to_string(ptr.use_count()) + "        ";
  }
  spdlog::get(oo::LOG)->trace("promoteCell(): Cache is   [{}]", exteriorCache);
  spdlog::get(oo::LOG)->trace("promoteCell(): Counts are [{}]", counts);
#endif
}

const CellCache::InteriorBuffer &CellCache::interiors() const {
  return mInteriors;
}

const CellCache::ExteriorBuffer &CellCache::exteriors() const {
  return mExteriors;
}

CellCache::InvalidationWrapper::InvalidationWrapper(CellCache *cache,
                                                    CellCache::WorldPtr ptr)
    : mCache(cache), mPtr(std::move(ptr)) {}

bool CellCache::InvalidationWrapper::invalidate() const noexcept {
  // The World has left the cache so all exterior cells belonging to it must
  // be removed from the, even if there is another copy of the World
  // alive somewhere (since it's destruction will only destroy the near cells,
  // not necessarily the cached ones).
  if (!mPtr) return false;
  auto p = [&](auto &cellPtr) {
    // Can't check BaseIds without going through a resolver
    return cellPtr->getSceneManager() == mPtr->getSceneManager();
  };
  mCache->mExteriors.erase(std::remove_if(mCache->mExteriors.begin(),
                                          mCache->mExteriors.end(),
                                          p),
                           mCache->mExteriors.end());
  return true;
}

CellCache::InvalidationWrapper::~InvalidationWrapper() {
  invalidate();
}

CellCache::InvalidationWrapper &
CellCache::InvalidationWrapper::operator=(CellCache::InvalidationWrapper &&other) noexcept {
  if (this != &other) {
    invalidate();
    mCache = other.mCache;
    mPtr = std::move(other.mPtr);
  }

  return *this;
}

const CellCache::WorldPtr &
CellCache::InvalidationWrapper::get() const noexcept {
  return mPtr;
}

void CellCache::push_back(const WorldPtr &world) {
  std::scoped_lock lock{mMutex};
  mWorlds.push_back(InvalidationWrapper(this, world));
}

CellCache::WorldPtr CellCache::getWorld(oo::BaseId id) const {
  std::scoped_lock lock{mMutex};
  const auto isId = [id](const auto &w) { return w.get()->getBaseId() == id; };

  auto it{std::find_if(mWorlds.begin(), mWorlds.end(), isId)};
  if (it != mWorlds.end()) return it->get();

  return nullptr;
}

std::vector<CellCache::WorldPtr> CellCache::worlds() const {
  std::vector<WorldPtr> v{};
  std::unique_lock lock{mMutex};
  v.reserve(mWorlds.size());
  std::transform(mWorlds.begin(),
                 mWorlds.end(),
                 std::back_inserter(v),
                 [](const auto &w) {
                   return w.get();
                 });

  return v;
}

void CellCache::promoteWorld(oo::BaseId id) {
  std::scoped_lock lock{mMutex};
  const auto isId = [id](const auto &w) { return w.get()->getBaseId() == id; };

  auto it{std::find_if(mWorlds.begin(), mWorlds.end(), isId)};
  if (it != mWorlds.end() && std::next(it) != mWorlds.end()) {
    std::rotate(it, std::next(it), mWorlds.end());
  }
}

} // namespace oo