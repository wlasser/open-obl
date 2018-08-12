#include <istream>
#include <fstream>
#include <iostream>
#include <string>
#include "save_state.hpp"
#include "formid.hpp"
#include "records.hpp"
#include "record.hpp"
#include "io/string.hpp"

bool SaveState::saveScreenshotPPM(const char *filename) {
  std::ofstream s(filename);
  // Header
  s << "P6\n"
    << screenshot.width << ' '
    << screenshot.height << "\n255\n";
  // Data
  s.write(reinterpret_cast<char *>(screenshot.data),
          screenshot.width * screenshot.height * 3);
  return true;
}

SaveState::SaveState(std::istream &s) {
  /// File header
  char fileId[12];
  s.get(fileId, 13);
  std::string fileIdStr(fileId);
  if (fileIdStr != "TES4SAVEGAME") {
    throw std::runtime_error("Invalid file signature");
  }
  s.read(reinterpret_cast<char *>(&majorVersion), sizeof(majorVersion));
  s.read(reinterpret_cast<char *>(&minorVersion), sizeof(minorVersion));
  s.read(reinterpret_cast<char *>(&exeTime), SystemTimeSize);

  /// Save game header
  s.read(reinterpret_cast<char *>(&headerVersion), sizeof(headerVersion));
  s.read(reinterpret_cast<char *>(&saveHeaderSize), sizeof(saveHeaderSize));
  s.read(reinterpret_cast<char *>(&saveNum), sizeof(saveNum));

  pcName = io::readBzString(s);
  s.read(reinterpret_cast<char *>(&pcLevel), sizeof(pcLevel));
  pcLocationStr = io::readBzString(s);
  s.read(reinterpret_cast<char *>(&gameDays), sizeof(float));
  s.read(reinterpret_cast<char *>(&gameTicks), sizeof(gameTicks));
  s.read(reinterpret_cast<char *>(&gameTime), SystemTimeSize);

  uint32_t screenshotSize = 0;
  s.read(reinterpret_cast<char *>(&screenshotSize), 4);
  s.read(reinterpret_cast<char *>(&screenshot.width), 4);
  s.read(reinterpret_cast<char *>(&screenshot.height), 4);
  screenshotSize -= 8;
  screenshot.data = new uint8_t[screenshotSize];
  s.read(reinterpret_cast<char *>(screenshot.data), screenshotSize);

  /// Plugins
  s.read(reinterpret_cast<char *>(&pluginsNum), 1);
  for (int i = 0; i < pluginsNum; ++i) {
    plugins.push_back(io::readBString(s));
  }

  /// Globals
  s.read(reinterpret_cast<char *>(&formIdsOffset), sizeof(formIdsOffset));
  s.read(reinterpret_cast<char *>(&recordsNum), sizeof(recordsNum));
  s.read(reinterpret_cast<char *>(&nextObjectId), sizeof(nextObjectId));
  s.read(reinterpret_cast<char *>(&worldId), sizeof(worldId));
  s.read(reinterpret_cast<char *>(&worldX), sizeof(worldX));
  s.read(reinterpret_cast<char *>(&worldY), sizeof(worldY));
  s.read(reinterpret_cast<char *>(&pcLocation), PCLocationSize);
  s.read(reinterpret_cast<char *>(&globalsNum), sizeof(globalsNum));
  if (globalsNum > 0) {
    for (int i = 0; i < globalsNum; ++i) {
      Global g{};
      s.read(reinterpret_cast<char *>(&g), GlobalSize);
      globals.push_back(g);
    }
  }
  s.read(reinterpret_cast<char *>(&tesClassSize), sizeof(tesClassSize));
  s.read(reinterpret_cast<char *>(&numDeathCounts), sizeof(numDeathCounts));
  if (numDeathCounts > 0) {
    for (auto i = 0u; i < numDeathCounts; ++i) {
      DeathCount d{};
      s.read(reinterpret_cast<char *>(&d), DeathCountSize);
      deathCounts.push_back(d);
    }
  }
  s.read(reinterpret_cast<char *>(&gameModeSeconds), sizeof(gameModeSeconds));
  s.read(reinterpret_cast<char *>(&processesSize), sizeof(processesSize));
  if (processesSize > 0) {
    processesData = new uint8_t[processesSize];
    s.read(reinterpret_cast<char *>(processesData), processesSize);
  }
  s.read(reinterpret_cast<char *>(&specEventSize), sizeof(specEventSize));
  if (specEventSize > 0) {
    specEventData = new uint8_t[specEventSize];
    s.read(reinterpret_cast<char *>(specEventData), specEventSize);
  }
  s.read(reinterpret_cast<char *>(&weatherSize), sizeof(weatherSize));
  if (weatherSize > 0) {
    weatherData = new uint8_t[weatherSize];
    s.read(reinterpret_cast<char *>(weatherData), weatherSize);
  }

  s.read(reinterpret_cast<char *>(&playerCombatCount),
         sizeof(playerCombatCount));
  s.read(reinterpret_cast<char *>(&createdNum), sizeof(createdNum));

  for (uint32_t i = 0; i < createdNum; ++i) {
    using namespace record;
    // Sometimes there is a single null byte
    std::string type = peekRecordType(s);
    if (type == "ALCH") {
      ALCH rec(raw::ALCH(), ALCH::Flag::None, 0, 0);
      s >> rec;
      std::cout << rec;
    }
  }
}
