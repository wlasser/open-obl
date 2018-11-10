#include "esp_coordinator.hpp"
#include "game_settings.hpp"

namespace esp {

std::vector<fs::Path> getMasters(const fs::Path &espFilename) {
  const auto &gameSettings{GameSettings::getSingleton()};
  const fs::Path dataPath{gameSettings.get("General.SLocalMasterPath", "Data")};

  std::ifstream esp(espFilename.sysPath(), std::ifstream::binary);
  auto masters{record::readRecord<record::TES4>(esp).masters};

  std::vector<fs::Path> paths(masters.size());
  std::transform(masters.begin(), masters.end(), paths.begin(),
                 [&dataPath](const record::raw::TES4::Master &entry) {
                   std::string masterBasename{entry.master.data};
                   return dataPath / fs::Path{masterBasename};
                 });
  return paths;
}

void EspCoordinator::invalidateEsp(Streams::iterator it) {
  for (auto &entry : mLoadOrder) {
    if (entry.it == it) entry.it = mStreams.end();
  }
}

void EspCoordinator::openStreamForEsp(EspEntry &esp, Streams::iterator it) {
  invalidateEsp(it);
  it->stream.close();
  it->stream.open(esp.filename.sysPath(), std::ifstream::binary);
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

std::optional<int> EspCoordinator::getModIndex(fs::Path modName) const {
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

FormId EspCoordinator::translateFormId(FormId id, int modIndex) const {
  const int localIndex{static_cast<int>((id & 0xff'000000) >> 24u)};
  if (localIndex >= mLoadOrder[modIndex].localLoadOrder.size()) {
    spdlog::get(settings::log)->critical(
        "FormId 0x{:0>8x} belongs to a non-dependent mod");
    throw std::runtime_error("FormId refers to a non-dependent mod");
  }
  const int globalIndex{mLoadOrder[modIndex].localLoadOrder[localIndex]};
  return (static_cast<unsigned int>(globalIndex) << 24u) | (id & 0x00'ffffff);
}

EspCoordinator::ReadHeaderResult
EspCoordinator::readRecordHeader(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->pos) it->stream.seekg(seekPos, std::ifstream::beg);
  return {translateFormIds(record::readRecordHeader(it->stream), modIndex),
      it->pos = it->stream.tellg()};
};

EspCoordinator::ReadHeaderResult
EspCoordinator::skipRecord(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->pos) it->stream.seekg(seekPos, std::ifstream::beg);
  return {translateFormIds(record::skipRecord(it->stream), modIndex),
      it->pos = it->stream.tellg()};
};

uint32_t EspCoordinator::peekRecordType(int modIndex, SeekPos seekPos) {
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

uint32_t EspAccessor::peekRecordType() {
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

template<>
BaseId EspCoordinator::translateFormIds(BaseId rec, int modIndex) const {
  return BaseId{translateFormId(FormId{rec}, modIndex)};
}

template<>
RefId EspCoordinator::translateFormIds(RefId rec, int modIndex) const {
  return RefId{translateFormId(FormId{rec}, modIndex)};
}

template<>
FormId EspCoordinator::translateFormIds(FormId rec, int modIndex) const {
  return translateFormId(rec, modIndex);
}

template<>
record::raw::DATA_MGEF
EspCoordinator::translateFormIds(record::raw::DATA_MGEF rec,
                                 int modIndex) const {
  rec.light = translateFormIds(rec.light, modIndex);
  rec.effectShader = translateFormIds(rec.effectShader, modIndex);
  rec.enchantEffect = translateFormIds(rec.enchantEffect, modIndex);
  rec.castingSound = translateFormIds(rec.castingSound, modIndex);
  rec.boltSound = translateFormIds(rec.boltSound, modIndex);
  rec.hitSound = translateFormIds(rec.hitSound, modIndex);
  rec.areaSound = translateFormIds(rec.areaSound, modIndex);
  return rec;
}

template<>
record::raw::DNAM
EspCoordinator::translateFormIds(record::raw::DNAM rec, int modIndex) const {
  rec.m = translateFormIds(rec.m, modIndex);
  rec.f = translateFormIds(rec.f, modIndex);
  return rec;
}

template<>
record::raw::ENAM
EspCoordinator::translateFormIds(record::raw::ENAM rec, int modIndex) const {
  for (auto &id : rec.eyes) {
    id = translateFormIds(id, modIndex);
  }
  return rec;
}

template<>
record::raw::HNAM
EspCoordinator::translateFormIds(record::raw::HNAM rec, int modIndex) const {
  for (auto &id : rec.hair) {
    id = translateFormIds(id, modIndex);
  }
  return rec;
}

template<>
record::raw::SCIT
EspCoordinator::translateFormIds(record::raw::SCIT rec, int modIndex) const {
  rec.id = translateFormIds(rec.id, modIndex);
  return rec;
}

template<>
record::raw::VNAM
EspCoordinator::translateFormIds(record::raw::VNAM rec, int modIndex) const {
  rec.m = translateFormIds(rec.m, modIndex);
  rec.f = translateFormIds(rec.f, modIndex);
  return rec;
}

template<>
record::raw::XESP
EspCoordinator::translateFormIds(record::raw::XESP rec, int modIndex) const {
  rec.parent = translateFormIds(rec.parent, modIndex);
  return rec;
}

template<>
record::raw::XLOC
EspCoordinator::translateFormIds(record::raw::XLOC rec, int modIndex) const {
  rec.key = translateFormIds(rec.key, modIndex);
  return rec;
}

template<>
record::raw::XNAM
EspCoordinator::translateFormIds(record::raw::XNAM rec, int modIndex) const {
  rec.factionID = translateFormIds(rec.factionID, modIndex);
  return rec;
}

template<>
record::raw::RACE
EspCoordinator::translateFormIds(record::raw::RACE rec, int modIndex) const {
  for (auto &power : rec.powers) {
    power.data = translateFormIds(power.data, modIndex);
  }
  for (auto &relation : rec.relations) {
    relation.data = translateFormIds(relation.data, modIndex);
  }
  if (rec.voices) {
    rec.voices->data = translateFormIds(rec.voices->data, modIndex);
  }
  if (rec.defaultHair) {
    rec.defaultHair->data = translateFormIds(rec.defaultHair->data, modIndex);
  }
  rec.hair.data = translateFormIds(rec.hair.data, modIndex);
  rec.eyes.data = translateFormIds(rec.eyes.data, modIndex);

  return rec;
}

template<>
record::raw::MGEF
EspCoordinator::translateFormIds(record::raw::MGEF rec, int modIndex) const {
  rec.data.data = translateFormIds(rec.data.data, modIndex);
  return rec;
}

template<>
record::raw::LTEX
EspCoordinator::translateFormIds(record::raw::LTEX rec, int modIndex) const {
  for (auto &grass : rec.potentialGrasses) {
    grass.data = translateFormIds(grass.data, modIndex);
  }
  return rec;
}

template<>
record::raw::BSGN
EspCoordinator::translateFormIds(record::raw::BSGN rec, int modIndex) const {
  for (auto &spell : rec.spells) {
    spell.data = translateFormIds(spell.data, modIndex);
  }
  return rec;
}

template<>
record::raw::ACTI
EspCoordinator::translateFormIds(record::raw::ACTI rec, int modIndex) const {
  if (rec.script) {
    rec.script->data = translateFormIds(rec.script->data, modIndex);
  }
  if (rec.sound) {
    rec.sound->data = translateFormIds(rec.sound->data, modIndex);
  }
  return rec;
}

template<>
record::raw::DOOR
EspCoordinator::translateFormIds(record::raw::DOOR rec, int modIndex) const {
  if (rec.script) {
    rec.script->data = translateFormIds(rec.script->data, modIndex);
  }
  if (rec.openSound) {
    rec.openSound->data = translateFormIds(rec.openSound->data, modIndex);
  }
  if (rec.closeSound) {
    rec.closeSound->data = translateFormIds(rec.closeSound->data, modIndex);
  }
  if (rec.loopSound) {
    rec.loopSound->data = translateFormIds(rec.loopSound->data, modIndex);
  }
  for (auto &teleport : rec.randomTeleports) {
    teleport.data = translateFormIds(teleport.data, modIndex);
  }
  return rec;
}

template<>
record::raw::LIGH
EspCoordinator::translateFormIds(record::raw::LIGH rec, int modIndex) const {
  if (rec.itemScript) {
    rec.itemScript->data = translateFormIds(rec.itemScript->data, modIndex);
  }
  if (rec.sound) {
    rec.sound->data = translateFormIds(rec.sound->data, modIndex);
  }
  return rec;
}

template<>
record::raw::MISC
EspCoordinator::translateFormIds(record::raw::MISC rec, int modIndex) const {
  if (rec.itemScript) {
    rec.itemScript->data = translateFormIds(rec.itemScript->data, modIndex);
  }
  return rec;
}

template<>
record::raw::ALCH
EspCoordinator::translateFormIds(record::raw::ALCH rec, int modIndex) const {
  if (rec.itemScript) {
    rec.itemScript->data = translateFormIds(rec.itemScript->data, modIndex);
  }
  return rec;
}

template<>
record::raw::CELL
EspCoordinator::translateFormIds(record::raw::CELL rec, int modIndex) const {
  if (rec.owner) {
    rec.owner->data = translateFormIds(rec.owner->data, modIndex);
  }
  if (rec.ownershipGlobal) {
    rec.ownershipGlobal->data =
        translateFormIds(rec.ownershipGlobal->data, modIndex);
  }
  if (rec.ownershipRank) {
    rec.ownershipRank->data =
        translateFormIds(rec.ownershipRank->data, modIndex);
  }
  if (rec.climate) {
    rec.climate->data = translateFormIds(rec.climate->data, modIndex);
  }
  if (rec.water) {
    rec.water->data = translateFormIds(rec.water->data, modIndex);
  }
  return rec;
}

template<>
record::raw::REFR
EspCoordinator::translateFormIds(record::raw::REFR rec, int modIndex) const {
  if (rec.parent) {
    rec.parent->data = translateFormIds(rec.parent->data, modIndex);
  }
  if (rec.target) {
    rec.target->data = translateFormIds(rec.target->data, modIndex);
  }
  if (rec.owner) {
    rec.owner->data = translateFormIds(rec.owner->data, modIndex);
  }
  if (rec.ownershipGlobal) {
    rec.ownershipGlobal->data =
        translateFormIds(rec.ownershipGlobal->data, modIndex);
  }
  if (rec.ownershipRank) {
    rec.ownershipRank->data =
        translateFormIds(rec.ownershipRank->data, modIndex);
  }
  if (rec.teleportParent) {
    rec.teleportParent->data =
        translateFormIds(rec.teleportParent->data, modIndex);
  }
  if (rec.lockInfo) {
    rec.lockInfo->data = translateFormIds(rec.lockInfo->data, modIndex);
  }

  return rec;
}

template<>
record::RecordHeader
EspCoordinator::translateFormIds(record::RecordHeader rec, int modIndex) const {
  rec.id = translateFormIds(rec.id, modIndex);
  return rec;
}

} // namespace esp