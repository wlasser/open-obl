#include "esp_coordinator.hpp"

namespace esp {

void EspCoordinator::invalidateEsp(Streams::iterator it) {
  for (auto &entry : mLoadOrder) {
    if (entry.it == it) entry.it = mStreams.end();
  }
}

void EspCoordinator::openStreamForEsp(EspEntry &esp, Streams::iterator it) {
  invalidateEsp(it);
  it->stream.close();
  it->stream.open(esp.filename, std::ifstream::binary);
  it->pos = 0;
  esp.it = it;
}

auto EspCoordinator::getFirstClosedStream() -> Streams::iterator {
  return std::find_if(mStreams.begin(), mStreams.end(), [](const Stream &s) {
    return !s.stream.is_open();
  });
}

auto EspCoordinator::getAvailableStream(EspEntry &esp) -> Streams::iterator {
  static std::random_device rd{};
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution dist(0, MaxOpenStreams - 1);

  Streams::iterator streamIt{};
  if (streamIt = esp.it; streamIt == mStreams.end()) {
    if (streamIt = getFirstClosedStream(); streamIt == mStreams.end()) {
      streamIt = mStreams.begin() + dist(gen);
    }
    openStreamForEsp(esp, streamIt);
  }
  return streamIt;
}

EspCoordinator::EspCoordinator(EspCoordinator &&other) noexcept {
  std::unique_lock lock{other.mMutex};
  std::swap(mStreams, other.mStreams);
  std::swap(mLoadOrder, other.mLoadOrder);
}

EspCoordinator &EspCoordinator::operator=(EspCoordinator &&other) noexcept {
  if (this != &other) {
    std::scoped_lock lock{mMutex, other.mMutex};
    std::swap(mStreams, other.mStreams);
    std::swap(mLoadOrder, other.mLoadOrder);
  }
  return *this;
}

EspAccessor EspCoordinator::makeAccessor(int modIndex) {
  return EspAccessor(modIndex, this);
}

std::optional<int> EspCoordinator::getModIndex(std::string_view modName) const {
  const auto begin{mLoadOrder.begin()};
  const auto end{mLoadOrder.end()};
  const auto it{std::find_if(begin, end, [modName](const EspEntry &e) {
    return e.filename == modName;
  })};
  return it == end ? std::nullopt : std::optional<int>{it - begin};
}

int EspCoordinator::getNumMods() const {
  // mLoadOrder.size() <= MaxPlugins < std::numeric_limits<int>::max
  return static_cast<int>(mLoadOrder.size());
}

void EspCoordinator::close(int modIndex) {
  std::scoped_lock lock{mMutex};
  if (auto &stream{mLoadOrder[modIndex]}; stream.it != mStreams.end()) {
    stream.it->stream.close();
    stream.it = mStreams.end();
  }
}

EspCoordinator::ReadHeaderResult
EspCoordinator::readRecordHeader(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->pos) it->stream.seekg(seekPos, std::ifstream::beg);
  return {record::readRecordHeader(it->stream), it->pos = it->stream.tellg()};
};

EspCoordinator::ReadHeaderResult
EspCoordinator::skipRecord(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->pos) it->stream.seekg(seekPos, std::ifstream::beg);
  return {record::skipRecord(it->stream), it->pos = it->stream.tellg()};
};

std::string EspCoordinator::peekRecordType(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->pos) it->stream.seekg(seekPos, std::ifstream::beg);
  return record::peekRecordType(it->stream);
};

EspCoordinator::ReadResult<record::Group>
EspCoordinator::readGroup(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->pos) it->stream.seekg(seekPos, std::ifstream::beg);
  record::Group g{};
  it->stream >> g;
  return {g, it->pos = it->stream.tellg()};
}

EspCoordinator::SeekPos
EspCoordinator::skipGroup(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->pos) it->stream.seekg(seekPos, std::ifstream::beg);
  record::skipGroup(it->stream);
  return it->stream.tellg();
}

std::optional<record::Group::GroupType>
EspCoordinator::peekGroupType(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->pos) it->stream.seekg(seekPos, std::ifstream::beg);
  return record::peekGroupType(it->stream);
}

EspAccessor::ReadHeaderResult EspAccessor::readRecordHeader() {
  auto r{mCoordinator->readRecordHeader(mIndex, mPos)};
  mPos = r.end;
  return r;
}

EspAccessor::ReadHeaderResult EspAccessor::skipRecord() {
  auto r{mCoordinator->skipRecord(mIndex, mPos)};
  mPos = r.end;
  return r;
}

std::string EspAccessor::peekRecordType() {
  return mCoordinator->peekRecordType(mIndex, mPos);
}

EspAccessor::ReadResult<record::Group> EspAccessor::readGroup() {
  auto r{mCoordinator->readGroup(mIndex, mPos)};
  mPos = r.end;
  return r;
}

void EspAccessor::skipGroup() {
  mPos = mCoordinator->skipGroup(mIndex, mPos);
}

std::optional<record::Group::GroupType> EspAccessor::peekGroupType() {
  return mCoordinator->peekGroupType(mIndex, mPos);
}

} // namespace esp
