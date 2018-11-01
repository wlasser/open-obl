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

template<class InputIt>
EspCoordinator::EspCoordinator(InputIt first, InputIt last) {
  auto out{std::back_inserter(mLoadOrder)};
  std::transform(first, last, out, [this](std::string name) {
    return EspEntry(std::move(name), mStreams.end());
  });
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

} // namespace esp