#include "persistent_reference_locator.hpp"
#include <mutex>

namespace oo {

tl::optional<oo::CellIndex>
PersistentReferenceLocator::getCellIndex(oo::RefId refId) const noexcept {
  std::unique_lock lock{mMutex};

  //C++20: if (mLocations.contains(refId)) {
  if (mLocations.find(refId) != mLocations.end()) {
    const auto loc{mLocations.at(refId)};
    if (loc.mWrldId != oo::BaseId{0}) {
      return tl::make_optional<oo::CellIndex>(loc.mCellIndex);
    }
  }

  return tl::nullopt;
}

tl::optional<oo::BaseId>
PersistentReferenceLocator::getWorldspace(oo::RefId refId) const noexcept {
  std::unique_lock lock{mMutex};

  //C++20: if (mLocations.contains(refId)) {
  if (mLocations.find(refId) != mLocations.end()) {
    const auto loc{mLocations.at(refId)};
    if (loc.mWrldId != oo::BaseId{0}) return loc.mWrldId;
  }

  return tl::nullopt;
}

tl::optional<oo::BaseId>
PersistentReferenceLocator::getCell(oo::RefId refId) const noexcept {
  std::unique_lock lock{mMutex};

  //C++20: if (mLocations.contains(refId)) {
  if (mLocations.find(refId) != mLocations.end()) {
    const auto loc{mLocations.at(refId)};
    if (loc.mCellId != oo::BaseId{0}) return loc.mCellId;
  }

  return tl::nullopt;
}

std::unordered_set<oo::RefId>
PersistentReferenceLocator::getRecordsInCell(oo::BaseId cellId) const noexcept {
  std::unique_lock lock{mMutex};

  std::unordered_set<oo::RefId> out{};
  for (const auto &[k, v] : mLocations) {
    if (v.mCellId == cellId) out.insert(k);
  }

  return out;
}

std::unordered_set<oo::RefId>
PersistentReferenceLocator::getRecordsInCell(oo::CellIndex cellIndex) const noexcept {
  std::unique_lock lock{mMutex};

  std::unordered_set<oo::RefId> out{};
  for (const auto &[k, v] : mLocations) {
    if (v.mCellIndex == cellIndex) out.insert(k);
  }

  return out;
}

void PersistentReferenceLocator::insert(oo::RefId refId,
                                        oo::BaseId cellId) noexcept {
  mLocations.insert_or_assign(refId, Location(cellId));
}

void PersistentReferenceLocator::insert(oo::RefId refId,
                                        oo::BaseId wrldId,
                                        oo::CellIndex cellIndex) noexcept {
  mLocations.insert_or_assign(refId, Location(wrldId, cellIndex));
}

} // namespace oo
