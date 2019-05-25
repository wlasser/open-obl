#include "esp/esp_coordinator.hpp"
#include "config/game_settings.hpp"
#include "record/records.hpp"
#include <fstream>

namespace oo {

std::vector<oo::Path> getMasters(const oo::Path &espFilename) {
  const auto &gameSettings{GameSettings::getSingleton()};
  const oo::Path dataPath{gameSettings.get("General.SLocalMasterPath", "Data")};

  std::ifstream esp(espFilename.sysPath(), std::ifstream::binary);
  auto masters{record::readRecord<record::TES4>(esp).masters};

  std::vector<oo::Path> paths(masters.size());
  std::transform(masters.begin(), masters.end(), paths.begin(),
                 [&dataPath](const record::raw::TES4::Master &entry) {
                   std::string masterBasename{entry.master.data};
                   return dataPath / oo::Path{masterBasename};
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
  streamIt->stream.clear();
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
  return EspAccessor(modIndex, gsl::make_not_null(this));
}

std::optional<int> EspCoordinator::getModIndex(const oo::Path &modName) const {
  std::scoped_lock lock{mMutex};
  const auto begin{mLoadOrder.begin()};
  const auto end{mLoadOrder.end()};
  const auto it{std::find_if(begin, end, [&modName](const EspEntry &e) {
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
  const int localIndex{static_cast<int>((id & 0xff'000000u) >> 24u)};
  if (static_cast<std::size_t>(localIndex)
      >= mLoadOrder[modIndex].localLoadOrder.size()) {
    spdlog::get(oo::LOG)->critical(
        "FormId 0x{:0>8x} belongs to a non-dependent mod", id);
    throw std::runtime_error("FormId refers to a non-dependent mod");
  }
  const int globalIndex{mLoadOrder[modIndex].localLoadOrder[localIndex]};
  return (static_cast<unsigned int>(globalIndex) << 24u) | (id & 0x00'ffffffu);
}

//===----------------------------------------------------------------------===//
// EspCoordinator input method implementations
//===----------------------------------------------------------------------===//

EspCoordinator::ReadHeaderResult
EspCoordinator::readRecordHeader(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->stream.tellg()) {
    it->stream.seekg(seekPos, std::ifstream::beg);
  }
  return {translateFormIds(record::readRecordHeader(it->stream), modIndex),
          it->stream.tellg()};
};

EspCoordinator::ReadHeaderResult
EspCoordinator::skipRecord(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->stream.tellg()) {
    it->stream.seekg(seekPos, std::ifstream::beg);
  }
  return {translateFormIds(record::skipRecord(it->stream), modIndex),
          it->stream.tellg()};
};

uint32_t EspCoordinator::peekRecordType(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->stream.tellg()) {
    it->stream.seekg(seekPos, std::ifstream::beg);
  }
  return record::peekRecordType(it->stream);
};

BaseId EspCoordinator::peekBaseId(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->stream.tellg()) {
    it->stream.seekg(seekPos, std::ifstream::beg);
  }
  return record::peekBaseOfReference(it->stream);
}

EspCoordinator::ReadResult<record::Group>
EspCoordinator::readGroup(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->stream.tellg()) {
    it->stream.seekg(seekPos, std::ifstream::beg);
  }
  record::Group g{};
  it->stream >> g;
  return {g, it->stream.tellg()};
}

EspCoordinator::SeekPos
EspCoordinator::skipGroup(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->stream.tellg()) {
    it->stream.seekg(seekPos, std::ifstream::beg);
  }
  record::skipGroup(it->stream);
  return it->stream.tellg();
}

std::optional<record::Group::GroupType>
EspCoordinator::peekGroupType(int modIndex, SeekPos seekPos) {
  std::scoped_lock lock{mMutex};
  auto it{getAvailableStream(mLoadOrder[modIndex])};
  if (seekPos != it->stream.tellg()) {
    it->stream.seekg(seekPos, std::ifstream::beg);
  }
  return record::peekGroupType(it->stream);
}

//===----------------------------------------------------------------------===//
// EspAccessor implementations
//===----------------------------------------------------------------------===//

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

BaseId EspAccessor::peekBaseId() {
  return mCoordinator->peekBaseId(mIndex, mPos);
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

//===----------------------------------------------------------------------===//
// translateFormIds implementations
//===----------------------------------------------------------------------===//

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

template<> record::raw::Effect
EspCoordinator::translateFormIds(record::raw::Effect rec, int modIndex) const {
  if (rec.script) {
    rec.script->data = translateFormIds(rec.script->data, modIndex);
  }
  return rec;
}

template<> record::raw::ATXT
EspCoordinator::translateFormIds(record::raw::ATXT rec, int modIndex) const {
  rec.id = translateFormIds(rec.id, modIndex);
  return rec;
}

template<> record::raw::BTXT
EspCoordinator::translateFormIds(record::raw::BTXT rec, int modIndex) const {
  rec.id = translateFormIds(rec.id, modIndex);
  return rec;
}

template<> record::raw::CNTO
EspCoordinator::translateFormIds(record::raw::CNTO rec, int modIndex) const {
  rec.id = translateFormIds(rec.id, modIndex);
  return rec;
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

template<> record::raw::GNAM_WATR
EspCoordinator::translateFormIds(record::raw::GNAM_WATR rec,
                                 int modIndex) const {
  rec.daytimeVariant = translateFormIds(rec.daytimeVariant, modIndex);
  rec.nighttimeVariant = translateFormIds(rec.nighttimeVariant, modIndex);
  rec.underwaterVariant = translateFormIds(rec.underwaterVariant, modIndex);

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

template<> record::raw::SNAM_NPC_
EspCoordinator::translateFormIds(record::raw::SNAM_NPC_ rec,
                                 int modIndex) const {
  rec.factionId = translateFormIds(rec.factionId, modIndex);
  return rec;
}

template<> record::raw::SNAM_WTHR
EspCoordinator::translateFormIds(record::raw::SNAM_WTHR rec,
                                 int modIndex) const {
  rec.soundId = translateFormIds(rec.soundId, modIndex);
  return rec;
}

template<>
record::raw::VNAM
EspCoordinator::translateFormIds(record::raw::VNAM rec, int modIndex) const {
  rec.m = translateFormIds(rec.m, modIndex);
  rec.f = translateFormIds(rec.f, modIndex);
  return rec;
}

template<> record::raw::VTEX
EspCoordinator::translateFormIds(record::raw::VTEX rec, int modIndex) const {
  for (auto &vtex : rec) vtex = translateFormIds(vtex, modIndex);
  return rec;
}

template<> record::raw::WLST
EspCoordinator::translateFormIds(record::raw::WLST rec, int modIndex) const {
  for (auto &weather : rec.weathers) {
    weather.formId = translateFormIds(weather.formId, modIndex);
  }
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
  rec.factionId = translateFormIds(rec.factionId, modIndex);
  return rec;
}

template<> record::raw::XTEL
EspCoordinator::translateFormIds(record::raw::XTEL rec, int modIndex) const {
  rec.destinationId = translateFormIds(rec.destinationId, modIndex);
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

template<> record::raw::ENCH
EspCoordinator::translateFormIds(record::raw::ENCH rec, int modIndex) const {
  for (auto &effect : rec.effects) {
    effect.data = translateFormIds(effect.data, modIndex);
  }
  return rec;
}

template<> record::raw::SPEL
EspCoordinator::translateFormIds(record::raw::SPEL rec, int modIndex) const {
  for (auto &effect : rec.effects) {
    effect.data = translateFormIds(effect.data, modIndex);
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
record::raw::CONT
EspCoordinator::translateFormIds(record::raw::CONT rec, int modIndex) const {
  if (rec.openSound) {
    rec.openSound->data = translateFormIds(rec.openSound->data, modIndex);
  }
  if (rec.closeSound) {
    rec.closeSound->data = translateFormIds(rec.closeSound->data, modIndex);
  }
  if (rec.script) {
    rec.script->data = translateFormIds(rec.script->data, modIndex);
  }
  for (auto &item : rec.items) {
    item = translateFormIds(item, modIndex);
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
record::raw::FLOR
EspCoordinator::translateFormIds(record::raw::FLOR rec, int modIndex) const {
  if (rec.script) {
    rec.script->data = translateFormIds(rec.script->data, modIndex);
  }
  if (rec.ingredient) {
    rec.ingredient->data = translateFormIds(rec.ingredient->data, modIndex);
  }
  return rec;
}

template<>
record::raw::FURN
EspCoordinator::translateFormIds(record::raw::FURN rec, int modIndex) const {
  if (rec.script) {
    rec.script->data = translateFormIds(rec.script->data, modIndex);
  }
  return rec;
}

template<> record::raw::NPC_
EspCoordinator::translateFormIds(record::raw::NPC_ rec, int modIndex) const {
  for (auto &faction : rec.factions) {
    faction.data = translateFormIds(faction.data, modIndex);
  }
  if (rec.deathItem) {
    rec.deathItem->data = translateFormIds(rec.deathItem->data, modIndex);
  }
  rec.race.data = translateFormIds(rec.race.data, modIndex);
  for (auto &spell : rec.spells) {
    spell.data = translateFormIds(spell.data, modIndex);
  }
  if (rec.script) {
    rec.script->data = translateFormIds(rec.script->data, modIndex);
  }
  for (auto &item : rec.items) {
    item.data = translateFormIds(item.data, modIndex);
  }
  for (auto &pkg : rec.aiPackages) {
    pkg.data = translateFormIds(pkg.data, modIndex);
  }
  rec.clas.data = translateFormIds(rec.clas.data, modIndex);
  if (rec.hair) {
    rec.hair->data = translateFormIds(rec.hair->data, modIndex);
  }
  if (rec.eyes) {
    rec.eyes->data = translateFormIds(rec.eyes->data, modIndex);
  }
  if (rec.combatStyle) {
    rec.combatStyle->data = translateFormIds(rec.combatStyle->data, modIndex);
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

template<> record::raw::WTHR
EspCoordinator::translateFormIds(record::raw::WTHR rec, int modIndex) const {
  for (auto &sound : rec.sounds) {
    sound.data = translateFormIds(sound.data, modIndex);
  }
  return rec;
}

template<> record::raw::CLMT
EspCoordinator::translateFormIds(record::raw::CLMT rec, int modIndex) const {
  if (rec.weatherList) {
    rec.weatherList->data = translateFormIds(rec.weatherList->data, modIndex);
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
record::raw::WRLD
EspCoordinator::translateFormIds(record::raw::WRLD rec, int modIndex) const {
  if (rec.parentWorldspace) {
    rec.parentWorldspace->data =
        translateFormIds(rec.parentWorldspace->data, modIndex);
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
record::raw::LAND
EspCoordinator::translateFormIds(record::raw::LAND rec, int modIndex) const {
  for (auto &btxt : rec.quadrantTexture) {
    btxt.data = translateFormIds(btxt.data, modIndex);
  }
  for (auto &[atxt, vtxt] : rec.fineTextures) {
    atxt.data = translateFormIds(atxt.data, modIndex);
  }
  if (rec.coarseTextures) {
    rec.coarseTextures->data =
        translateFormIds(rec.coarseTextures->data, modIndex);
  }

  return rec;
}

template<> record::raw::WATR
EspCoordinator::translateFormIds(record::raw::WATR rec, int modIndex) const {
  if (rec.variants) {
    rec.variants->data = translateFormIds(rec.variants->data, modIndex);
  }
  return rec;
}

template<>
record::raw::REFR_ACTI
EspCoordinator::translateFormIds(record::raw::REFR_ACTI rec,
                                 int modIndex) const {
  if (rec.parent) {
    rec.parent->data = translateFormIds(rec.parent->data, modIndex);
  }
  rec.baseId = translateFormIds(rec.baseId, modIndex);
  if (rec.target) {
    rec.target->data = translateFormIds(rec.target->data, modIndex);
  }

  return rec;
}

template<>
record::raw::REFR_CONT
EspCoordinator::translateFormIds(record::raw::REFR_CONT rec,
                                 int modIndex) const {
  if (rec.parent) {
    rec.parent->data = translateFormIds(rec.parent->data, modIndex);
  }
  rec.baseId = translateFormIds(rec.baseId, modIndex);
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
  if (rec.lockInfo) {
    rec.lockInfo->data = translateFormIds(rec.lockInfo->data, modIndex);
  }
  return rec;
}

template<>
record::raw::REFR_DOOR
EspCoordinator::translateFormIds(record::raw::REFR_DOOR rec,
                                 int modIndex) const {
  if (rec.parent) {
    rec.parent->data = translateFormIds(rec.parent->data, modIndex);
  }
  rec.baseId = translateFormIds(rec.baseId, modIndex);
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
record::raw::REFR_LIGH
EspCoordinator::translateFormIds(record::raw::REFR_LIGH rec,
                                 int modIndex) const {
  if (rec.parent) {
    rec.parent->data = translateFormIds(rec.parent->data, modIndex);
  }
  rec.baseId = translateFormIds(rec.baseId, modIndex);

  return rec;
}

template<>
record::raw::REFR_MISC
EspCoordinator::translateFormIds(record::raw::REFR_MISC rec,
                                 int modIndex) const {
  if (rec.parent) {
    rec.parent->data = translateFormIds(rec.parent->data, modIndex);
  }
  rec.baseId = translateFormIds(rec.baseId, modIndex);
  if (rec.ownershipGlobal) {
    rec.ownershipGlobal->data =
        translateFormIds(rec.ownershipGlobal->data, modIndex);
  }
  if (rec.ownershipRank) {
    rec.ownershipRank->data =
        translateFormIds(rec.ownershipRank->data, modIndex);
  }

  return rec;
}

template<>
record::raw::REFR_STAT
EspCoordinator::translateFormIds(record::raw::REFR_STAT rec,
                                 int modIndex) const {
  if (rec.parent) {
    rec.parent->data = translateFormIds(rec.parent->data, modIndex);
  }
  rec.baseId = translateFormIds(rec.baseId, modIndex);

  return rec;
}

template<>
record::raw::REFR_FLOR
EspCoordinator::translateFormIds(record::raw::REFR_FLOR rec,
                                 int modIndex) const {
  if (rec.parent) {
    rec.parent->data = translateFormIds(rec.parent->data, modIndex);
  }
  rec.baseId = translateFormIds(rec.baseId, modIndex);

  return rec;
}

template<>
record::raw::REFR_FURN
EspCoordinator::translateFormIds(record::raw::REFR_FURN rec,
                                 int modIndex) const {
  if (rec.parent) {
    rec.parent->data = translateFormIds(rec.parent->data, modIndex);
  }
  rec.baseId = translateFormIds(rec.baseId, modIndex);

  return rec;
}

template<> record::raw::REFR_NPC_
EspCoordinator::translateFormIds(record::raw::REFR_NPC_ rec,
                                 int modIndex) const {
  if (rec.parent) {
    rec.parent->data = translateFormIds(rec.parent->data, modIndex);
  }
  if (rec.merchantContainer) {
    rec.merchantContainer->data =
        translateFormIds(rec.merchantContainer->data, modIndex);
  }
  if (rec.mount) {
    rec.mount->data = translateFormIds(rec.mount->data, modIndex);
  }
  rec.baseId = translateFormIds(rec.baseId, modIndex);

  return rec;
}

template<>
record::RecordHeader
EspCoordinator::translateFormIds(record::RecordHeader rec, int modIndex) const {
  rec.id = translateFormIds(rec.id, modIndex);
  return rec;
}

} // namespace oo
