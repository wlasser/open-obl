#include "formid.hpp"
#include "io/memstream.hpp"
#include "io/read_bytes.hpp"
#include "io/string.hpp"
#include "record/record.hpp"
#include "records.hpp"
#include "save_state.hpp"
#include <OgreDataStream.h>
#include <fstream>
#include <istream>
#include <string>

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

SaveState::SaveState(std::istream &is) {
  // UESP says this is a 12 byte string without a null-terminator, followed by
  // a major version number byte, which is conveniently always zero. Might it
  // simply be a null-terminator? It is safe to assume so regardless.
  std::string headerStr{};
  io::readBytes(is, headerStr);
  if (headerStr != "TES4SAVEGAME") {
    throw std::runtime_error("Invalid file signature");
  }
  io::readBytes(is, mVersion);
  io::readBytes(is, mExeTime);

  io::readBytes(is, mHeaderVersion);

  // Size in bytes of the remaining save game header. This is not needed.
  uint32_t headerSize{};
  io::readBytes(is, headerSize);

  io::readBytes(is, mSaveNumber);

  mPCName = io::readBzString(is);
  io::readBytes(is, mPCLevel);
  mPCCellName = io::readBzString(is);

  io::readBytes(is, mGameDaysPassed);
  io::readBytes(is, mGameTicksPassed);
  io::readBytes(is, mSaveTime);

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
  mScreenshot.loadRawData(stream, screenshotWidth, screenshotHeight,
                          screenshotDepth, Ogre::PixelFormat::PF_BYTE_RGB);

  io::readBytes(is, mNumPlugins);
  for (uint8_t i = 0; i < mNumPlugins; ++i) {
    mPlugins.emplace_back(io::readBString(is));
  }

  io::readBytes(is, mFormIdsOffset);
  io::readBytes(is, mNumChangeRecords);
  io::readBytes(is, mNextFormId);

  io::readBytes(is, mWorldspaceId);
  io::readBytes(is, mWorldPos);
  io::readBytes(is, mPCCellId);
  io::readBytes(is, mPCPosition);

  uint16_t numGlobals{};
  io::readBytes(is, numGlobals);
  mGlobals.assign(numGlobals, {});
  for (auto &global : mGlobals) {
    io::readBytes(is, global);
  }

  // Size of mDeathCounts and mGameModeSeconds.
  uint16_t tesClassSize{};
  io::readBytes(is, tesClassSize);

  uint32_t numDeathCounts{};
  io::readBytes(is, numDeathCounts);
  mDeathCounts.assign(numDeathCounts, {});
  for (auto &deathCount : mDeathCounts) {
    io::readBytes(is, deathCount.mActor);
    io::readBytes(is, deathCount.mCount);
  }

  io::readBytes(is, mGameModeSecondsPassed);

  uint16_t processesDataSize{};
  io::readBytes(is, processesDataSize);
  io::readBytes(is, mProcessesData, processesDataSize);

  uint16_t specEventDataSize{};
  io::readBytes(is, specEventDataSize);
  io::readBytes(is, mSpecEventData, specEventDataSize);

  uint16_t weatherDataSize{};
  io::readBytes(is, weatherDataSize);
  io::readBytes(is, mWeatherData, weatherDataSize);

  io::readBytes(is, mPlayerCombatCount);

  io::readBytes(is, mNumCreatedRecords);
}
