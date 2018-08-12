#ifndef SAVE_STATE_HPP
#define SAVE_STATE_HPP

#include <string>
#include <cctype>
#include <memory>
#include <vector>
#include "system_time.hpp"
#include "formid.hpp"
#include "io_util.hpp"

class SaveState {

  struct Screenshot {
    uint32_t width;
    uint32_t height;
    uint8_t *data;
  };

  struct PCLocation {
    FormID cell;
    float x, y, z;
  };
  static const std::size_t PCLocationSize = sizeof(FormID) + 3 * sizeof(float);

  struct Global {
    IRef iref;
    float value;
  };
  static const std::size_t GlobalSize = sizeof(IRef) + sizeof(float);

  struct DeathCount {
    // IRef to base form
    IRef actor;
    // Number of times an instance of this actor has died
    uint16_t deathCount;
  };
  static const std::size_t DeathCountSize = sizeof(IRef) + sizeof(uint16_t);

 public:

  /// File header
  uint8_t majorVersion;
  uint8_t minorVersion;
  // Time when game executable was last modified
  struct SystemTime exeTime;

  /// Save game header
  uint32_t headerVersion;
  // Size in bytes of the save game header not including this variable
  // This is actually redundant as the header size is easily inferred
  uint32_t saveHeaderSize;
  uint32_t saveNum;
  std::string pcName;
  uint16_t pcLevel;
  // Player's current cell
  std::string pcLocationStr;
  // Days that have passed in game
  float gameDays;
  // Ticks elapsed during gameplay
  uint32_t gameTicks;
  // Time that the save file was created
  struct SystemTime gameTime;
  // Screenshot at time of save
  struct Screenshot screenshot;

  /// Plugins
  // Number of plugins including masters
  uint8_t pluginsNum;
  // Plugin names in load order
  std::vector<std::string> plugins;

  /// Global
  // The absolute address of formIdsNum later in the file
  uint32_t formIdsOffset;
  // Number of change records
  uint32_t recordsNum;
  // Number of next dynamic formid i.e. 0xff000000
  FormID nextObjectId;
  // Worldspace information
  FormID worldId;
  uint32_t worldX;
  uint32_t worldY;
  // Player location
  struct PCLocation pcLocation;
  // Array of global variables
  uint16_t globalsNum;
  std::vector<Global> globals;
  // Size of upcoming data form death count and game mode time.
  // This is unnecessary but it is given in the file so we store it
  uint16_t tesClassSize;
  // Array of death counts for actors
  uint32_t numDeathCounts;
  std::vector<DeathCount> deathCounts;
  // Seconds elapsed in game mode (menus closed)
  float gameModeSeconds;
  // Processes data
  uint16_t processesSize;
  uint8_t *processesData;
  // Spectator event data
  uint16_t specEventSize;
  uint8_t *specEventData;
  // Weather data
  uint16_t weatherSize;
  uint8_t *weatherData;
  // Number of actors in combat with the player
  uint32_t playerCombatCount;
  // Number of created items?
  uint32_t createdNum;

  // Constructor
  explicit SaveState(std::istream &);
  ~SaveState() {
    delete[] screenshot.data;
    if (processesSize > 0) delete[] processesData;
    if (specEventSize > 0) delete[] specEventData;
    if (weatherSize > 0) delete[] weatherData;
  }

  // Save the screenshot as a ppm file
  bool saveScreenshotPPM(const char *filename);
};

#endif // SAVE_STATE_HPP
