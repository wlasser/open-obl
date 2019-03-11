#include "cell_cache.hpp"
#include "world_cache.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include <algorithm>
#include <mutex>

namespace oo {

CellCache::GetResult CellCache::get(oo::BaseId id) const {
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
}

void CellCache::promote(oo::BaseId id) {
  std::scoped_lock lock{mMutex};
  const auto isId = [id](const auto &ptr) { return ptr->getBaseId() == id; };

  auto intIt{std::find_if(mInteriors.begin(), mInteriors.end(), isId)};
  if (intIt != mInteriors.end() && intIt + 1 != mInteriors.end()) {
    std::rotate(intIt, intIt + 1, mInteriors.end());
    return;
  }

  auto extIt{std::find_if(mExteriors.end(), mExteriors.end(), isId)};
  if (extIt != mExteriors.end() && extIt + 1 != mExteriors.end()) {
    std::rotate(extIt, extIt + 1, mExteriors.end());
  }
}

const CellCache::InteriorBuffer &CellCache::interiors() const {
  return mInteriors;
}

const CellCache::ExteriorBuffer &CellCache::exteriors() const {
  return mExteriors;
}

void WorldCache::push_back(const WorldPtr &world) {
  std::scoped_lock lock{mMutex};
  mWorlds.push_back(world);
}

WorldCache::GetResult WorldCache::get(oo::BaseId id) const {
  std::scoped_lock lock{mMutex};
  const auto isId = [id](const auto &ptr) { return ptr->getBaseId() == id; };

  auto it{std::find_if(mWorlds.begin(), mWorlds.end(), isId)};
  if (it != mWorlds.end()) return *it;

  return GetResult();
}

const WorldCache::WorldBuffer &WorldCache::worlds() const { return mWorlds; }

void WorldCache::promote(oo::BaseId id) {
  std::scoped_lock lock{mMutex};
  const auto isId = [id](const auto &ptr) { return ptr->getBaseId() == id; };

  auto it{std::find_if(mWorlds.begin(), mWorlds.end(), isId)};
  if (it != mWorlds.end() && it + 1 != mWorlds.end()) {
    std::rotate(it, it + 1, mWorlds.end());
  }
}

} // namespace oo