#ifndef OPENOBLIVION_ESP_COORDINATOR_HPP
#define OPENOBLIVION_ESP_COORDINATOR_HPP

#include "fs/path.hpp"
#include "record/group.hpp"
#include "record/io.hpp"
#include "record/records_fwd.hpp"
#include "record/reference_records.hpp"
#include "record/subrecords.hpp"
#include "settings.hpp"
#include <boost/fiber/mutex.hpp>
#include <gsl/gsl>
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

/// Abstraction layer for access to esp files.
/// The `oo::EspCoordinator` acts a gateway for the esp files loaded by the
/// program, abstracting away the load order and providing thread-safe IO of the
/// records.
/// \todo Output is not currently supported, and needs to be added.
/// \see \ref md_doc_esp
class EspCoordinator {
 private:
  /// The maximum number of streams open at any one time.
  constexpr static inline int MaxOpenStreams{16};
  /// The maximum number of plugins loaded at any one time.
  /// This is very much a hard limit of the system, and increasing number would
  /// require changing the size of `oo::FormId` and similar.
  constexpr static inline int MaxPlugins{0xff};

  struct Stream {
    std::ifstream stream{};
  };

  using Streams = std::array<Stream, MaxOpenStreams>;

  /// Array of streams that can be used for reading esps.
  Streams mStreams{};

  struct EspEntry {
    /// Path of the esp file.
    /// \invariant This shall not be modified after construction, except to be
    ///            moved from.
    oo::Path filename;
    /// Local load order of the esp given as indices into `mLoadOrder`. The last
    /// element is the index of this esp.
    /// \invariant This shall not be modified after construction, except to be
    ///            moved from.
    std::vector<int> localLoadOrder;
    /// Iterator to the stream in `mStreams` that is currently open to this
    /// file, or one-past-the-end if no stream is open to this file.
    Streams::iterator it;

    EspEntry() = delete;
    ~EspEntry() = default;

    template<class InputIt>
    EspEntry(oo::Path name, Streams::iterator it,
             InputIt loadOrderBegin, InputIt loadOrderEnd)
        : filename(std::move(name)),
          localLoadOrder(loadOrderBegin, loadOrderEnd),
          it(it) {}

    EspEntry(const EspEntry &) = delete;
    EspEntry &operator=(const EspEntry &) = delete;

    EspEntry(EspEntry &&other) noexcept = default;
    EspEntry &operator=(EspEntry &&) noexcept = default;
  };

  /// The mod index of a mod is its position in this list. Must always contain
  /// `MaxPlugins` elements or fewer.
  /// \invariant The size and ordering of elements of the load order shall not
  ///            be modified after construction. In particular, the invariants
  ///            of `EspEntry` ensure that only the `it` member of each entry
  ///            may be modified, unless the entire `EspCoordinator` is moved
  ///            from. In particular, this means that all properties of the
  ///            load order except the state of the `it` member are free to be
  ///            read without locking `mMutex`.
  std::vector<EspEntry> mLoadOrder{};

  mutable boost::fibers::mutex mMutex{};

  /// Find the esp wih the given stream and point its iterator to the end,
  /// marking it as no longer loaded.
  void invalidateEsp(Streams::iterator it);

  /// Invalidate any esp with the given stream, open the stream to the given
  /// esp, and point the esp entry to it.
  void openStreamForEsp(EspEntry &esp, Streams::iterator it);
  //C++20: [[expects: mStreams.begin() <= it && it < mStreams.end()]];

  /// Return the first closed entry in `mStreams`.
  Streams::iterator getFirstClosedStream();

  /// If the esp already has a stream associated to it, return it. If not,
  /// open and return the first closed stream. If all streams are already open,
  /// choose a random stream, open it to the esp, and return.
  Streams::iterator getAvailableStream(EspEntry &esp);

 public:
  /// `first` and `last` are iterators to a collection of `oo::Path`s equal to
  /// the mod filenames sorted in load order from 'load first' to 'load last'.
  template<class ForwardIt, class = std::enable_if_t<
      std::is_base_of_v<std::forward_iterator_tag,
                        typename std::iterator_traits<ForwardIt>::iterator_category>
          && std::is_convertible_v<const oo::Path &,
                                   typename std::iterator_traits<ForwardIt>::reference>>>
  EspCoordinator(ForwardIt first, ForwardIt last);

  EspCoordinator(const EspCoordinator &) = delete;
  EspCoordinator &operator=(const EspCoordinator &) = delete;

  EspCoordinator(EspCoordinator &&) noexcept;
  EspCoordinator &operator=(EspCoordinator &&) noexcept;

  ~EspCoordinator() = default;
  EspCoordinator() = delete;

  EspAccessor makeAccessor(int modIndex);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  /// Return the mod index (i.e. position in the load order) of the given mod.
  std::optional<int> getModIndex(const oo::Path &modName) const;

  /// Returns the number of mods in the load order
  int getNumMods() const;

  /// If the given mod has an open stream, close it and invalidate its iterator
  /// to make the stream available for another mod. If the mod does not have an
  /// open stream, do nothing. Calling this method is never required, but it is
  /// polite to do so if the mod's stream is no longer needed.
  void close(int modIndex);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  /// Take a `FormId` whose mod index is local to the given mod and return the
  /// same `FormId` but with its mod index translated to the global load order.
  FormId translateFormId(FormId id, int modIndex) const;
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  using SeekPos = std::ifstream::pos_type;

  /// The result of a read operation. Contains both the read value and the
  /// position of the stream indicator one-past-the-end of the read value.
  template<class T>
  struct ReadResult {
    T value{};
    SeekPos end{};
  };

  /// The result of a header read operation. Contains both the read header and
  /// the position of the stream indicator one-past-the-end of the read value.
  struct ReadHeaderResult {
    [[maybe_unused]] record::RecordHeader header{};
    SeekPos end{};
  };

  /// \name Read Operations
  /// Each read operation takes the global mod index of the esp to read from,
  /// and a position in the esp file to move the stream indicator to before
  /// reading. Often this will be the position returned by a previous read
  /// operation, but it need not be. It is necessary that the caller be
  /// responsible for where they are reading, as multiple callers can read the
  /// file and would all likely expect their reads to be sequential.
  /// @{

  /// Read a `record::Record<T>`.
  template<class T>
  ReadResult<T> readRecord(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  /// Read only the `record::RecordHeader` of the next record.
  ReadHeaderResult readRecordHeader(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  /// Skip over the next record, of whatever type, returning its header when
  /// doing so.
  ReadHeaderResult skipRecord(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  /// Return the type of the next `record::Record`, but don't advance the stream
  /// indicator. If `seekPos` is not positioned at the start of a
  /// `record::Record`, then return zero.
  uint32_t peekRecordType(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  /// Return the `oo::BaseId` of the next `record::Record`, but don't advance
  /// the stream indicator.
  BaseId peekBaseId(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  /// Read a `record::Group`.
  ReadResult<record::Group> readGroup(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  /// Skip over the next `record::Group`.
  SeekPos skipGroup(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  /// Return the type of next `record::Group`, but don't advance the stream.
  /// If the `seekPos` is not positioned at the start of a `record::Group`, then
  /// return an empty optional.
  std::optional<record::Group::GroupType>
  peekGroupType(int modIndex, SeekPos seekPos);
  //C++20: [[expects: 0 <= modIndex && modIndex < getNumMods()]];

  /// @}

  /// \name FormId Translation Functions
  /// These functions take objects which have `oo::FormId` objects as members
  /// and return a new object with `translateFormId` applied to each of the
  /// members.
  /// @{
  template<class T> T translateFormIds(T rec, int modIndex) const;

  template<> BaseId translateFormIds(BaseId rec, int modIndex) const;
  template<> RefId translateFormIds(RefId rec, int modIndex) const;
  template<> FormId translateFormIds(FormId rec, int modIndex) const;

  template<> record::raw::Effect
  translateFormIds(record::raw::Effect rec, int modIndex) const;

  template<> record::raw::ATXT
  translateFormIds(record::raw::ATXT rec, int modIndex) const;
  template<> record::raw::BTXT
  translateFormIds(record::raw::BTXT rec, int modIndex) const;
  template<> record::raw::CNTO
  translateFormIds(record::raw::CNTO rec, int modIndex) const;
  template<> record::raw::DATA_MGEF
  translateFormIds(record::raw::DATA_MGEF rec, int modIndex) const;
  template<> record::raw::DNAM
  translateFormIds(record::raw::DNAM rec, int modIndex) const;
  template<> record::raw::ENAM
  translateFormIds(record::raw::ENAM rec, int modIndex) const;
  template<> record::raw::GNAM_WATR
  translateFormIds(record::raw::GNAM_WATR rec, int modIndex) const;
  template<> record::raw::HNAM
  translateFormIds(record::raw::HNAM rec, int modIndex) const;
  template<> record::raw::SCIT
  translateFormIds(record::raw::SCIT rec, int modIndex) const;
  template<> record::raw::SNAM_NPC_
  translateFormIds(record::raw::SNAM_NPC_ rec, int modIndex) const;
  template<> record::raw::SNAM_WTHR
  translateFormIds(record::raw::SNAM_WTHR rec, int modIndex) const;
  template<> record::raw::VNAM
  translateFormIds(record::raw::VNAM rec, int modIndex) const;
  template<> record::raw::VTEX
  translateFormIds(record::raw::VTEX rec, int modIndex) const;
  template<> record::raw::WLST
  translateFormIds(record::raw::WLST rec, int modIndex) const;
  template<> record::raw::XESP
  translateFormIds(record::raw::XESP rec, int modIndex) const;
  template<> record::raw::XLOC
  translateFormIds(record::raw::XLOC rec, int modIndex) const;
  template<> record::raw::XNAM
  translateFormIds(record::raw::XNAM rec, int modIndex) const;
  template<> record::raw::XTEL
  translateFormIds(record::raw::XTEL rec, int modIndex) const;

  template<> record::raw::RACE
  translateFormIds(record::raw::RACE rec, int modIndex) const;
  template<> record::raw::MGEF
  translateFormIds(record::raw::MGEF rec, int modIndex) const;
  template<> record::raw::LTEX
  translateFormIds(record::raw::LTEX rec, int modIndex) const;
  template<> record::raw::ENCH
  translateFormIds(record::raw::ENCH rec, int modIndex) const;
  template<> record::raw::SPEL
  translateFormIds(record::raw::SPEL rec, int modIndex) const;
  template<> record::raw::BSGN
  translateFormIds(record::raw::BSGN rec, int modIndex) const;
  template<> record::raw::ACTI
  translateFormIds(record::raw::ACTI rec, int modIndex) const;
  template<> record::raw::DOOR
  translateFormIds(record::raw::DOOR rec, int modIndex) const;
  template<> record::raw::LIGH
  translateFormIds(record::raw::LIGH rec, int modIndex) const;
  template<> record::raw::MISC
  translateFormIds(record::raw::MISC rec, int modIndex) const;
  template<> record::raw::NPC_
  translateFormIds(record::raw::NPC_ rec, int modIndex) const;
  template<> record::raw::ALCH
  translateFormIds(record::raw::ALCH rec, int modIndex) const;
  template<> record::raw::WTHR
  translateFormIds(record::raw::WTHR rec, int modIndex) const;
  template<> record::raw::CLMT
  translateFormIds(record::raw::CLMT rec, int modIndex) const;
  template<> record::raw::CELL
  translateFormIds(record::raw::CELL rec, int modIndex) const;
  template<> record::raw::WRLD
  translateFormIds(record::raw::WRLD rec, int modIndex) const;
  template<> record::raw::LAND
  translateFormIds(record::raw::LAND rec, int modIndex) const;
  template<> record::raw::WATR
  translateFormIds(record::raw::WATR rec, int modIndex) const;

  template<> record::raw::REFR_ACTI
  translateFormIds(record::raw::REFR_ACTI rec, int modIndex) const;
  template<> record::raw::REFR_DOOR
  translateFormIds(record::raw::REFR_DOOR rec, int modIndex) const;
  template<> record::raw::REFR_LIGH
  translateFormIds(record::raw::REFR_LIGH rec, int modIndex) const;
  template<> record::raw::REFR_MISC
  translateFormIds(record::raw::REFR_MISC rec, int modIndex) const;
  template<> record::raw::REFR_STAT
  translateFormIds(record::raw::REFR_STAT rec, int modIndex) const;
  template<> record::raw::REFR_NPC_
  translateFormIds(record::raw::REFR_NPC_ rec, int modIndex) const;

  template<class T, uint32_t c>
  record::Record<T, c>
  translateFormIds(record::Record<T, c> rec, int modIndex) const;

  template<>
  record::RecordHeader
  translateFormIds(record::RecordHeader rec, int modIndex) const;

  /// @}
};

/// Convenience class for reading a specific mod sequentially.
/// Instances of this class are produced by `EspCoordinator::makeAccessor` and
/// expose the same IO interface as `EspCoordinator` but restricted to
/// sequential access of a fixed mod file.
///
/// From a caller's point of view, IO is generally sequential, and so keeping
/// track of a mod index and stream position seems unnecessarily burdensome. An
/// added benefit of wrapping both those things is that passing around an
/// `EspAccessor` between functions allows abstracting away exactly *which* file
/// is being read; the caller may not care exactly where they are reading from,
/// only that they are reading a particular `record::Record`.
///
/// A single instance of `EspAccessor` is not fiber safe, however it *is* safe
/// for two different `EspAccessor`s accessing the same file to be used
/// concurrently.
class EspAccessor {
 private:
  friend EspCoordinator;

  int mIndex;
  gsl::not_null<EspCoordinator *> mCoordinator;
  EspCoordinator::SeekPos mPos{0};

  explicit EspAccessor(int modIndex,
                       gsl::not_null<EspCoordinator *> coordinator)
      : mIndex(modIndex), mCoordinator(coordinator) {}

 public:
  template<class T>
  using ReadResult = EspCoordinator::ReadResult<T>;
  using ReadHeaderResult = EspCoordinator::ReadHeaderResult;

  /// \name Read Operations
  /// \see oo::EspCoordinator
  /// @{
  template<class T> ReadResult<T> readRecord();

  ReadHeaderResult readRecordHeader();

  ReadHeaderResult skipRecord();

  uint32_t peekRecordType();

  BaseId peekBaseId();

  ReadResult<record::Group> readGroup();

  void skipGroup();

  std::optional<record::Group::GroupType> peekGroupType();
  /// @}
};

/// Load `espFilename`, read the `record::TES4` record, and return the names of
/// its masters. `espFilename` should be prefixed with the data folder. The
/// returned names will also be prefixed by the data folder.
std::vector<oo::Path> getMasters(const oo::Path &espFilename);

//===----------------------------------------------------------------------===//
// EspCoordinator template implementations
//===----------------------------------------------------------------------===//

template<class ForwardIt, class>
EspCoordinator::EspCoordinator(ForwardIt first, ForwardIt last) {
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
        loadOrder.push_back(std::distance(first, it));
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
T EspCoordinator::translateFormIds(T rec, int /*modIndex*/) const {
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
