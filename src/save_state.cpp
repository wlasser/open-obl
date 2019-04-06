#include "io/io.hpp"
#include "io/memstream.hpp"
#include "io/string.hpp"
#include "record/formid.hpp"
#include "record/record.hpp"
#include "record/records.hpp"
#include "save_state.hpp"
#include <OgreDataStream.h>
#include <fstream>
#include <istream>
#include <string>

namespace oo {

std::string SystemTime::toISO8601() const {
  std::stringstream s;
  s << year << '-'
    << std::setfill('0') << std::setw(2) << month << '-'
    << std::setfill('0') << std::setw(2) << day << 'T'
    << std::setfill('0') << std::setw(2) << hour << ':'
    << std::setfill('0') << std::setw(2) << minute << ':'
    << std::setfill('0') << std::setw(2) << second;
  return s.str();
}

//===----------------------------------------------------------------------===//
// EssAccessor implementations
//===----------------------------------------------------------------------===//

EssAccessor::ReadHeaderResult EssAccessor::readRecordHeader() {
  return {record::readRecordHeader(mIs), mIs.tellg()};
}

EssAccessor::ReadHeaderResult EssAccessor::skipRecord() {
  return {record::skipRecord(mIs), mIs.tellg()};
}

uint32_t EssAccessor::peekRecordType() {
  return record::peekRecordType(mIs);
}

oo::BaseId EssAccessor::peekBaseId() {
  return record::peekBaseOfReference(mIs);
}

//===----------------------------------------------------------------------===//
// EssVisitor
//===----------------------------------------------------------------------===//
namespace {

class EssVisitor {
 private:
  oo::BaseResolversRef mBaseCtx;
  std::vector<oo::BaseId> &mCreatedRecords;

  template<class R> void readRecordDefault(oo::EssAccessor &accessor) {
    const auto rec{accessor.readRecord<R>().value};
    const oo::BaseId baseId{rec.mFormId};
    oo::getResolver<R>(mBaseCtx).insertOrAssign(baseId, rec);
    mCreatedRecords.push_back(baseId);
  }

 public:
  explicit EssVisitor(std::vector<oo::BaseId> &createdRecords,
                      oo::BaseResolversRef baseCtx) noexcept
      : mBaseCtx(std::move(baseCtx)), mCreatedRecords(createdRecords) {}

  template<class R> void readRecord(oo::EssAccessor &accessor) {
    accessor.skipRecord();
  }

  template<> void readRecord<record::LIGH>(oo::EssAccessor &accessor) {
    readRecordDefault<record::LIGH>(accessor);
  }

  template<> void readRecord<record::MISC>(oo::EssAccessor &accessor) {
    readRecordDefault<record::MISC>(accessor);
  }

  template<> void readRecord<record::NPC_>(oo::EssAccessor &accessor) {
    readRecordDefault<record::NPC_>(accessor);
  }
};

} // namespace

//===----------------------------------------------------------------------===//
// SaveState implementations
//===----------------------------------------------------------------------===//

std::istream &operator>>(std::istream &is, oo::SaveState &sv) {
  // UESP says this is a 12 byte string without a null-terminator, followed by
  // a major version number byte, which is conveniently always zero. Might it
  // simply be a null-terminator? It is safe to assume so regardless.
  std::string headerStr{};
  io::readBytes(is, headerStr);
  if (headerStr != "TES4SAVEGAME") {
    throw std::runtime_error("Invalid file signature");
  }
  io::readBytes(is, sv.mVersion);
  io::readBytes(is, sv.mExeTime);

  io::readBytes(is, sv.mHeaderVersion);

  // Size in bytes of the remaining save game header. This is not needed.
  uint32_t headerSize{};
  io::readBytes(is, headerSize);

  io::readBytes(is, sv.mSaveNumber);

  sv.mPlayerName = io::readBzString(is);
  io::readBytes(is, sv.mPlayerLevel);
  sv.mPlayerCellName = io::readBzString(is);

  io::readBytes(is, sv.mGameDaysPassed);
  io::readBytes(is, sv.mGameTicksPassed);
  io::readBytes(is, sv.mSaveTime);

  // Entire size of the screenshot, *including* the width and height.
  uint32_t screenshotSize{0};
  io::readBytes(is, screenshotSize);

  uint32_t screenshotWidth{0};
  io::readBytes(is, screenshotWidth);

  uint32_t screenshotHeight{0};
  io::readBytes(is, screenshotHeight);

  std::vector<uint8_t> pixels(screenshotSize - 8u);
  is.read(reinterpret_cast<char *>(pixels.data()), pixels.size());
  auto stream{std::make_shared<Ogre::MemoryDataStream>(
      pixels.data(), pixels.size())};

  const uint32_t screenshotDepth{1u};
  sv.mScreenshot.loadRawData(stream, screenshotWidth, screenshotHeight,
                             screenshotDepth, Ogre::PixelFormat::PF_BYTE_RGB);

  io::readBytes(is, sv.mNumPlugins);
  for (uint8_t i = 0; i < sv.mNumPlugins; ++i) {
    sv.mPlugins.emplace_back(io::readBString(is));
  }

  io::readBytes(is, sv.mFormIdsOffset);
  io::readBytes(is, sv.mNumChangeRecords);
  io::readBytes(is, sv.mNextFormId);

  io::readBytes(is, sv.mWorldspaceId);
  io::readBytes(is, sv.mWorldPos);
  io::readBytes(is, sv.mPlayerCellId);
  io::readBytes(is, sv.mPlayerPosition);

  uint16_t numGlobals{};
  io::readBytes(is, numGlobals);
  io::readBytes(is, sv.mGlobals, numGlobals);

  // Size of mDeathCounts and mGameModeSeconds.
  uint16_t tesClassSize{};
  io::readBytes(is, tesClassSize);

  uint32_t numDeathCounts{};
  io::readBytes(is, numDeathCounts);
  sv.mDeathCounts.assign(numDeathCounts, {});
  for (auto &deathCount : sv.mDeathCounts) {
    io::readBytes(is, deathCount.mActor);
    io::readBytes(is, deathCount.mCount);
  }

  io::readBytes(is, sv.mGameModeSecondsPassed);

  uint16_t processesDataSize{};
  io::readBytes(is, processesDataSize);
  io::readBytes(is, sv.mProcessesData, processesDataSize);

  uint16_t specEventDataSize{};
  io::readBytes(is, specEventDataSize);
  io::readBytes(is, sv.mSpecEventData, specEventDataSize);

  uint16_t weatherDataSize{};
  io::readBytes(is, weatherDataSize);
  io::readBytes(is, sv.mWeatherData, weatherDataSize);

  io::readBytes(is, sv.mPlayerCombatCount);

  io::readBytes(is, sv.mNumCreatedRecords);

  // Following is a list of base records, but they are ungrouped. Records
  // that can appear include
  // - ALCH
  // - SPEL
  // - ARMO
  // - WEAP
  // - ENCH
  // - NPC_ (including the player character?)
  // - BOOK
  // - INGR
  // - MISC
  // - KEYM
  // - LIGH (e.g. torches)
  sv.mCreatedRecords.reserve(sv.mNumCreatedRecords);
  EssVisitor visitor(sv.mCreatedRecords, sv.mBaseCtx);
  EssAccessor accessor(is);
  for (std::size_t i = 0; i < sv.mNumCreatedRecords; ++i) {
    const auto recType{accessor.peekRecordType()};
    oo::readRecord(accessor, recType, visitor);
  }

  uint16_t quickKeysSize{};
  io::readBytes(is, quickKeysSize);
  for (std::size_t i = 0; quickKeysSize != 0; ++i) {
    uint8_t isSet{0};
    io::readBytes(is, isSet);
    if (isSet != 0) {
      io::readBytes(is, sv.mQuickKeys[i]);
      quickKeysSize -= 5u;
    } else {
      sv.mQuickKeys[i] = 0u;
      quickKeysSize -= 1u;
    }
  }

  uint16_t reticuleSize{};
  io::readBytes(is, reticuleSize);
  io::readBytes(is, sv.mReticuleData, reticuleSize);

  uint16_t interfaceSize{};
  io::readBytes(is, interfaceSize);
  io::readBytes(is, sv.mInterfaceData, interfaceSize);

  uint16_t regionSize{};
  io::readBytes(is, regionSize);
  uint16_t numRegions{};
  io::readBytes(is, numRegions);
  io::readBytes(is, sv.mRegions, numRegions);

  return is;
}

std::ostream &operator<<(std::ostream &os, const oo::SaveState &sv) {
  io::writeBytes(os, "TES4SAVEGAME");
  io::writeBytes(os, sv.mVersion);
  io::writeBytes(os, sv.mExeTime);
  io::writeBytes(os, sv.mHeaderVersion);

  const auto headerSize{sizeof(sv.mSaveNumber)
                            + (1u + sv.mPlayerName.size())
                            + sizeof(sv.mPlayerLevel)
                            + (1u + sv.mPlayerCellName.size())
                            + sizeof(sv.mGameDaysPassed)
                            + sizeof(sv.mGameTicksPassed)
                            + sizeof(sv.mSaveTime)
                            + (12u + sv.mScreenshot.getSize())};
  io::writeBytes(os, static_cast<uint32_t>(headerSize));

  io::writeBytes(os, sv.mSaveNumber);

  io::writeBzString(os, sv.mPlayerName);
  io::writeBytes(os, sv.mPlayerLevel);
  io::writeBzString(os, sv.mPlayerCellName);

  io::writeBytes(os, sv.mGameDaysPassed);
  io::writeBytes(os, sv.mGameTicksPassed);
  io::writeBytes(os, sv.mSaveTime);

  io::writeBytes(os, static_cast<uint32_t>(sv.mScreenshot.getSize()));
  io::writeBytes(os, static_cast<uint32_t>(sv.mScreenshot.getWidth()));
  io::writeBytes(os, static_cast<uint32_t>(sv.mScreenshot.getHeight()));
  os.write(reinterpret_cast<const char *>(sv.mScreenshot.getData()),
           sv.mScreenshot.getSize());

  io::writeBytes(os, sv.mNumPlugins);
  for (const auto &plugin : sv.mPlugins) io::writeBString(os, plugin);

  io::writeBytes(os, sv.mFormIdsOffset);
  io::writeBytes(os, sv.mNumChangeRecords);
  io::writeBytes(os, sv.mNextFormId);

  io::writeBytes(os, sv.mWorldspaceId);
  io::writeBytes(os, sv.mWorldPos);
  io::writeBytes(os, sv.mPlayerCellId);
  io::writeBytes(os, sv.mPlayerPosition);

  io::writeBytes(os, static_cast<uint16_t>(sv.mGlobals.size()));
  for (auto global : sv.mGlobals) io::writeBytes(os, global);

  const auto tesClassSize{sizeof(8u + 6u * sv.mDeathCounts.size())};
  io::writeBytes(os, static_cast<uint16_t>(tesClassSize));
  io::writeBytes(os, static_cast<uint32_t>(sv.mDeathCounts.size()));
  for (const auto &deathCount : sv.mDeathCounts) {
    io::writeBytes(os, deathCount.mActor);
    io::writeBytes(os, deathCount.mCount);
  }
  io::writeBytes(os, sv.mGameModeSecondsPassed);

  io::writeBytes(os, static_cast<uint16_t>(sv.mProcessesData.size()));
  os.write(reinterpret_cast<const char *>(sv.mProcessesData.data()),
           sv.mProcessesData.size());

  io::writeBytes(os, static_cast<uint16_t>(sv.mSpecEventData.size()));
  os.write(reinterpret_cast<const char *>(sv.mSpecEventData.data()),
           sv.mSpecEventData.size());

  io::writeBytes(os, static_cast<uint16_t>(sv.mWeatherData.size()));
  os.write(reinterpret_cast<const char *>(sv.mWeatherData.data()),
           sv.mWeatherData.size());

  io::writeBytes(os, sv.mPlayerCombatCount);


  // TODO: Write the save state created records.
  //io::writeBytes(os, sv.mNumCreatedRecords);
  io::writeBytes(os, static_cast<uint32_t>(0u));

  const auto quickKeysSize{std::accumulate(
      sv.mQuickKeys.begin(), sv.mQuickKeys.end(), 0u,
      [](std::size_t s, uint32_t qk) -> std::size_t {
        return s + (qk == 0u ? 1u : 5u);
      })};
  io::writeBytes(os, static_cast<uint16_t>(quickKeysSize));

  for (auto quickKey : sv.mQuickKeys) {
    if (quickKey == 0) {
      io::writeBytes(os, static_cast<uint8_t>(0u));
    } else {
      io::writeBytes(os, static_cast<uint8_t>(1u));
      io::writeBytes(os, quickKey);
    }
  }

  io::writeBytes(os, static_cast<uint16_t>(sv.mReticuleData.size()));
  os.write(reinterpret_cast<const char *>(sv.mReticuleData.data()),
           sv.mReticuleData.size());

  io::writeBytes(os, static_cast<uint16_t>(sv.mInterfaceData.size()));
  os.write(reinterpret_cast<const char *>(sv.mInterfaceData.data()),
           sv.mInterfaceData.size());

  io::writeBytes(os, static_cast<uint16_t>(4u + 8u * sv.mRegions.size()));
  io::writeBytes(os, static_cast<uint16_t>(sv.mRegions.size()));

  for (const auto &region : sv.mRegions) io::writeBytes(os, region);

  return os;
}

} // namespace oo
