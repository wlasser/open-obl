#ifndef OPENOBLIVION_ESP_COORDINATOR_HPP
#define OPENOBLIVION_ESP_COORDINATOR_HPP

#include "fs/path.hpp"
#include "record/group.hpp"
#include "record/io.hpp"
#include "record/records.hpp"
#include "settings.hpp"
#include <spdlog/spdlog.h>
#include <array>
#include <algorithm>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace oo {

class EspAccessor;

class EspCoordinator {
 private:
  // The maximum number of streams open at any one time.
  constexpr static inline int MaxOpenStreams{16};
  // The maximum number of plugins loaded at any one time.
  constexpr static inline int MaxPlugins{0xff};

  struct Stream {
    std::ifstream stream{};
  };

  using Streams = std::array<Stream, MaxOpenStreams>;

  // Array of streams that can be used for reading esps.
  Streams mStreams{};

  struct EspEntry {
    // Path of the esp file.
    oo::Path filename{};
    // Local load order of the esp given as indices into mLoadOrder. The last
    // element is the index of this esp.
    std::vector<int> localLoadOrder{};
    // Iterator to the stream in mStreams that is currently open to this file.
    Streams::iterator it{};

    EspEntry() = delete;
    template<class InputIt>
    EspEntry(oo::Path name, Streams::iterator it, InputIt loadOrderStart,
             InputIt loadOrderEnd) : filename(std::move(name)), it(it) {
      localLoadOrder.insert(localLoadOrder.end(), loadOrderStart, loadOrderEnd);
    }

    ~EspEntry() = default;

    EspEntry(const EspEntry &) = delete;
    EspEntry &operator=(const EspEntry &) = delete;

    EspEntry(EspEntry &&) noexcept = default;
    EspEntry &operator=(EspEntry &&) noexcept = default;
  };

  // The mod index of a mod is its position in this list. Must always contain
  // MaxPlugins elements or fewer.
  std::vector<EspEntry> mLoadOrder{};

  std::mutex mMutex{};

  // Find the esp wih the given stream and point its iterator to the end,
  // marking it as no longer loaded.
  void invalidateEsp(Streams::iterator it);

  // Invalidate any esp with the given stream, open the stream to the given esp,
  // and point the esp entry to it.
  void openStreamForEsp(EspEntry &esp, Streams::iterator it);
  //C++20: [[expects: mStreams.begin() <= it && it < mStreams.end()]];

  // Return the first closed entry in mStreams.
  Streams::iterator getFirstClosedStream();

  // If the esp already has a stream associated to it, return it. If not,
  // open and return the first closed stream. If all stream are already open,
  // choose a random stream, open it to the esp, and return.
  Streams::iterator getAvailableStream(EspEntry &esp);

 public:
  // first and last are iterators to a collection of oo::Paths equal to the mod
  // filenames sorted in load order from 'load first' to 'load last'.
  template<class InputIt>
  EspCoordinator(InputIt first, InputIt last);

  EspCoordinator(const EspCoordinator &) = delete;
  EspCoordinator &operator=(const EspCoordinator &) = delete;

  EspCoordinator(EspCoordinator &&) noexcept;
  EspCoordinator &operator=(EspCoordinator &&) noexcept;

  ~EspCoordinator() = default;

  EspAccessor makeAccessor(int modIndex);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  // Return the mod index (i.e. position in the load order) of the given mod.
  std::optional<int> getModIndex(oo::Path modName) const;

  // Returns the number of mods in the load order
  int getNumMods() const;

  // If the given mod has an open stream, close it and invalidate its iterator
  // to make the stream available for another mod. If the mod does not have an
  // open stream, do nothing. Calling this method is never required, but it is
  // polite to do so if the mod's stream is no longer needed.
  void close(int modIndex);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  // Take a FormId whose mod index is local to the given mod and return the same
  // FormId but with its mod index translated to the global load order.
  FormId translateFormId(FormId id, int modIndex) const;
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  using SeekPos = std::ifstream::pos_type;

  template<class T>
  struct ReadResult {
    T value{};
    SeekPos end{};
  };

  struct ReadHeaderResult {
    [[maybe_unused]] record::RecordHeader header{};
    SeekPos end{};
  };

  template<class T>
  ReadResult<T> readRecord(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  ReadHeaderResult readRecordHeader(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  ReadHeaderResult skipRecord(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  uint32_t peekRecordType(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  BaseId peekBaseId(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  ReadResult<record::Group> readGroup(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  SeekPos skipGroup(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  std::optional<record::Group::GroupType>
  peekGroupType(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  template<class T>
  T translateFormIds(T rec, int modIndex) const;

  template<>
  BaseId translateFormIds(BaseId rec, int modIndex) const;

  template<>
  RefId translateFormIds(RefId rec, int modIndex) const;

  template<>
  FormId translateFormIds(FormId rec, int modIndex) const;

  template<>
  record::raw::DATA_MGEF
  translateFormIds(record::raw::DATA_MGEF rec, int modIndex) const;

  template<>
  record::raw::DNAM translateFormIds(record::raw::DNAM rec, int modIndex) const;

  template<>
  record::raw::ENAM translateFormIds(record::raw::ENAM rec, int modIndex) const;

  template<>
  record::raw::HNAM translateFormIds(record::raw::HNAM rec, int modIndex) const;

  template<>
  record::raw::SCIT translateFormIds(record::raw::SCIT rec, int modIndex) const;

  template<>
  record::raw::VNAM translateFormIds(record::raw::VNAM rec, int modIndex) const;

  template<>
  record::raw::XESP translateFormIds(record::raw::XESP rec, int modIndex) const;

  template<>
  record::raw::XLOC translateFormIds(record::raw::XLOC rec, int modIndex) const;

  template<>
  record::raw::XNAM translateFormIds(record::raw::XNAM rec, int modIndex) const;

  template<>
  record::raw::RACE translateFormIds(record::raw::RACE rec, int modIndex) const;

  template<>
  record::raw::MGEF translateFormIds(record::raw::MGEF rec, int modIndex) const;

  template<>
  record::raw::LTEX translateFormIds(record::raw::LTEX rec, int modIndex) const;

  template<>
  record::raw::BSGN translateFormIds(record::raw::BSGN rec, int modIndex) const;

  template<>
  record::raw::ACTI translateFormIds(record::raw::ACTI rec, int modIndex) const;

  template<>
  record::raw::DOOR translateFormIds(record::raw::DOOR rec, int modIndex) const;

  template<>
  record::raw::LIGH translateFormIds(record::raw::LIGH rec, int modIndex) const;

  template<>
  record::raw::MISC translateFormIds(record::raw::MISC rec, int modIndex) const;

  template<>
  record::raw::CELL translateFormIds(record::raw::CELL rec, int modIndex) const;

  template<>
  record::raw::REFR_ACTI
  translateFormIds(record::raw::REFR_ACTI rec, int modIndex) const;

  template<>
  record::raw::REFR_DOOR
  translateFormIds(record::raw::REFR_DOOR rec, int modIndex) const;

  template<>
  record::raw::REFR_LIGH
  translateFormIds(record::raw::REFR_LIGH rec, int modIndex) const;

  template<>
  record::raw::REFR_MISC
  translateFormIds(record::raw::REFR_MISC rec, int modIndex) const;

  template<>
  record::raw::REFR_STAT
  translateFormIds(record::raw::REFR_STAT rec, int modIndex) const;

  template<class T, uint32_t c>
  record::Record<T, c>
  translateFormIds(record::Record<T, c> rec, int modIndex) const;

  template<>
  record::RecordHeader
  translateFormIds(record::RecordHeader rec, int modIndex) const;
};

class EspAccessor {
 private:
  friend EspCoordinator;

  int mIndex{};
  EspCoordinator *mCoordinator{};
  EspCoordinator::SeekPos mPos{0};

  explicit EspAccessor(int modIndex, EspCoordinator *coordinator)
      : mIndex(modIndex), mCoordinator(coordinator) {}

 public:
  template<class T>
  using ReadResult = EspCoordinator::ReadResult<T>;
  using ReadHeaderResult = EspCoordinator::ReadHeaderResult;

  template<class T>
  ReadResult<T> readRecord();

  ReadHeaderResult readRecordHeader();

  ReadHeaderResult skipRecord();

  uint32_t peekRecordType();

  BaseId peekBaseId();

  ReadResult<record::Group> readGroup();

  void skipGroup();

  std::optional<record::Group::GroupType> peekGroupType();
};

// Load espFilename, read the TES4 record, and return the names of its masters.
// espFilename should be prefixed with the data folder. The returned names will
// be prefixed by the data folder.
std::vector<oo::Path> getMasters(const oo::Path &espFilename);

template<class InputIt>
EspCoordinator::EspCoordinator(InputIt first, InputIt last) {
  auto out{std::back_inserter(mLoadOrder)};
  std::transform(first, last, out, [&](const oo::Path &childPath) {
    // Putting the child esp at the end of its own master list ensures that
    // the child esp appears last in its local load order.
    auto masters{getMasters(childPath)};
    masters.push_back(childPath);
    std::vector<int> loadOrder{};
    loadOrder.reserve(masters.size());

    for (const auto &master : masters) {
      if (const auto it{std::find(first, last, master)}; it != last) {
        loadOrder.push_back(it - first);
      } else {
        spdlog::get(oo::LOG)->critical(
            "{} depends on master {} which is not loaded",
            childPath.view(), master.view());
        throw std::runtime_error("Dependency not met");
      }
    }

    return EspEntry(childPath, mStreams.end(),
                    loadOrder.begin(), loadOrder.end());
  });
}

template<class T>
EspCoordinator::ReadResult<T>
EspCoordinator::readRecord(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->stream.tellg()) {
    it->stream.seekg(seekPos, std::ifstream::beg);
  }
  return {translateFormIds(record::readRecord<T>(it->stream), modIndex),
          it->stream.tellg()};
};

template<class T>
EspAccessor::ReadResult<T> EspAccessor::readRecord() {
  auto r{mCoordinator->readRecord<T>(mIndex, mPos)};
  mPos = r.end;
  return r;
}

template<class T>
T EspCoordinator::translateFormIds(T rec, int modIndex) const {
  return rec;
}

template<class T, uint32_t c>
record::Record<T, c>
EspCoordinator::translateFormIds(record::Record<T, c> rec, int modIndex) const {
  rec.mFormId = translateFormIds(rec.mFormId, modIndex);
  // WTF, I didn't expect this to actually work!
  T &rawRef{rec};
  rawRef.operator=(translateFormIds(rawRef, modIndex));
  return rec;
}

} // namespace oo

#endif // OPENOBLIVION_ESP_COORDINATOR_HPP
