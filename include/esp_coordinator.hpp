#ifndef OPENOBLIVION_ESP_COORDINATOR_HPP
#define OPENOBLIVION_ESP_COORDINATOR_HPP

#include "record/group.hpp"
#include "record/io.hpp"
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

namespace esp {

class EspAccessor;

class EspCoordinator {
 private:
  // The maximum number of streams open at any one time.
  constexpr static inline int MaxOpenStreams{16};
  // The maximum number of plugins loaded at any one time.
  constexpr static inline int MaxPlugins{0xff};

  struct Stream {
    std::ifstream stream{};
    std::ifstream::pos_type pos{};
  };

  using Streams = std::array<Stream, MaxOpenStreams>;

  // Array of streams that can be used for reading esps.
  Streams mStreams{};

  struct EspEntry {
    // Base name of the esp file.
    std::string filename{};
    // Iterator to the stream in mStreams that is currently open to this file.
    Streams::iterator it{};

    EspEntry() = delete;
    EspEntry(std::string name, Streams::iterator it)
        : filename(std::move(name)), it(it) {}

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
  // first and last are iterators to a collection of strings equal to the mod
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
  std::optional<int> getModIndex(std::string_view modName) const;

  // Returns the number of mods in the load order
  int getNumMods() const;

  // If the given mod has an open stream, close it and invalidate its iterator
  // to make the stream available for another mod. If the mod does not have an
  // open stream, do nothing. Calling this method is never required, but it is
  // polite to do so if the mod's stream is no longer needed.
  void close(int modIndex);
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

  std::string peekRecordType(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  ReadResult<record::Group> readGroup(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  SeekPos skipGroup(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  std::optional<record::Group::GroupType>
  peekGroupType(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];
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

  std::string peekRecordType();

  ReadResult<record::Group> readGroup();

  void skipGroup();

  std::optional<record::Group::GroupType> peekGroupType();
};

template<class InputIt>
EspCoordinator::EspCoordinator(InputIt first, InputIt last) {
  auto out{std::back_inserter(mLoadOrder)};
  std::transform(first, last, out, [this](std::string name) {
    return EspEntry(std::move(name), mStreams.end());
  });
}

template<class T>
EspCoordinator::ReadResult<T>
EspCoordinator::readRecord(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->pos) it->stream.seekg(seekPos, std::ifstream::beg);
  return {record::readRecord<T>(it->stream), it->pos = it->stream.tellg()};
};

template<class T>
EspAccessor::ReadResult<T> EspAccessor::readRecord() {
  auto r{mCoordinator->readRecord<T>(mIndex, mPos)};
  mPos = r.end;
  return r;
}

} // namespace esp

#endif // OPENOBLIVION_ESP_COORDINATOR_HPP
