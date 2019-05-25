#ifndef OPENOBL_SAVE_STATE_HPP
#define OPENOBL_SAVE_STATE_HPP

#include "cell_cache.hpp"
#include "esp/esp.hpp"
#include "io/io.hpp"
#include "record/formid.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/cell_resolver.hpp"
#include <OgreImage.h>
#include <cctype>
#include <filesystem>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace oo {

struct SystemTime : io::byte_direct_ioable_tag {
  uint16_t year;
  uint16_t month;
  uint16_t dayOfWeek;
  uint16_t day;
  uint16_t hour;
  uint16_t minute;
  uint16_t second;
  uint16_t millisecond;

  std::string toISO8601() const;
};
static_assert(sizeof(SystemTime) == 16u);

/// Return a path (either fully-qualified or relative to the executable) to the
/// directory containing save games.
std::filesystem::path getSaveDirectory();

class EssAccessor {
 private:
  friend class SaveState;

  std::istream &mIs;

 public:
  explicit EssAccessor(std::istream &is) : mIs(is) {}

  template<class T> using ReadResult = EspCoordinator::ReadResult<T>;
  using ReadHeaderResult = EspCoordinator::ReadHeaderResult;

  /// \name Read Operations
  /// @{
  template<class T> ReadResult<T> readRecord() {
    return {record::readRecord<T>(mIs), mIs.tellg()};
  }

  ReadHeaderResult readRecordHeader();

  ReadHeaderResult skipRecord();

  uint32_t peekRecordType();

  BaseId peekBaseId();

  ReadResult<record::Group> readGroup() = delete;

  void skipGroup() = delete;

  std::optional<record::Group::GroupType> peekGroupType() = delete;
  /// @}
};

class SaveState {
 private:
  struct Global : io::byte_direct_ioable_tag {
    oo::IRef iref{};
    /// This is broken in the same way as record::GLOB.
    float value{};
  };
  static_assert(sizeof(Global) == 8u);

  struct DeathCount {
    /// oo::IRef to base form
    oo::IRef mActor{};
    /// Number of times an instance of this actor has died
    uint16_t mCount{};
    // uint16_t padding;
  };

  struct Region : io::byte_direct_ioable_tag {
    oo::IRef iref{};
    uint32_t flags{};
  };
  static_assert(sizeof(Region) == 8u);

  /// Base resolvers to looup `oo::BaseId`s in and insert base records into.
  oo::BaseResolversRef mBaseCtx;

 public:
  explicit SaveState(oo::BaseResolversRef baseCtx)
      : mBaseCtx(std::move(baseCtx)) {}

  friend std::ostream &operator<<(std::ostream &os, const SaveState &sv);
  friend std::istream &operator>>(std::istream &is, SaveState &sv);

  /// Construct an `oo::CellRequest` to load the cell the player is in.
  oo::CellRequest makeCellRequest() const;

  /// \name File Header
  ///@{

  /// File format version.
  /// Always 125, though UESP says that 126 has been reported but unconfirmed.
  uint8_t mVersion{};

  /// Time when the game executable was last modified.
  SystemTime mExeTime{};

  ///@}

  /// \name Save Game Header
  ///@{

  /// Save game version.
  /// Should equal mVersion.
  uint32_t mHeaderVersion{};

  /// Number of save games for the character prior to this save.
  uint32_t mSaveNumber{};

  /// Player character's name.
  std::string mPlayerName{};

  /// Player character's level.
  uint16_t mPlayerLevel{};

  /// Name of the cell the player character is currently in.
  /// Specifically, the record::FULL of the current record::CELL.
  std::string mPlayerCellName{};

  /// Number of days that have passed in game.
  /// According to UESP this begins at 1.042, as the start time of the game is
  /// 1am on day 1, namely Morndas 27th of Last Seed.
  /// gameDaysPassed is therefore the amount of time, in days, that have passed
  /// since the epoch 12am Sundas 26th of Last Seed.
  float mGameDaysPassed{};

  /// Number of milliseconds elapsed while playing this save game.
  uint32_t mGameTicksPassed{};

  /// Time that the save file was created.
  struct SystemTime mSaveTime{};

  /// Screenshot of GameMode at time of save.
  Ogre::Image mScreenshot{};

  ///@}

  /// \name Plugins
  ///@{

  /// Number of active plugins, including masters.
  uint8_t mNumPlugins{};

  /// Plugin names in load order.
  /// Names are filepaths relative to General.SLocalMasterPath, e.g.
  /// 'Oblivion.esm'.
  std::vector<std::string> mPlugins{};

  ///@}

  /// \name Global
  /// @{

  /// Position in bytes of mNumFormIds from the start of the file.
  uint32_t mFormIdsOffset{};

  /// Number of entries in mChangeRecords
  uint32_t mNumChangeRecords{};

  /// Number of next dynamic FormId `0xffxxxxxx`
  oo::FormId mNextFormId{};

  /// oo::FormId of last world space the player was in before saving.
  /// If the player is in an interior cell then this is not necessarily the
  /// worldspace the player is currently in.
  oo::FormId mWorldspaceId{};

  /// Exterior cell grid position of the exterior cell the player is in.
  /// Specifically, the $(x,y)$ components of the record::XCLC of the current
  /// exterior record::CELL. This is present but meaningless if the player is
  /// not in an exterior cell.
  std::tuple<int32_t, int32_t> mWorldPos{};

  /// oo::FormId of the record::CELL the player is currently in.
  oo::FormId mPlayerCellId{};

  /// $(x,y,z)$ coordinates, in world units, of the player in the current
  /// record::CELL.
  std::tuple<float, float, float> mPlayerPosition{};

  /// Array of global variables.
  std::vector<Global> mGlobals{};

  /// List of death counts for actors.
  std::vector<DeathCount> mDeathCounts{};

  /// Number of seconds elapsed during GameMode
  float mGameModeSecondsPassed{};

  /// Processes data.
  /// \todo This needs to be decoded.
  std::vector<uint8_t> mProcessesData{};

  /// Spectator event data
  /// \todo This needs to be decoded.
  std::vector<uint8_t> mSpecEventData{};

  /// Weather data.
  /// \todo This needs to be decoded.
  std::vector<uint8_t> mWeatherData{};

  /// Number of actors in combat with the player.
  uint32_t mPlayerCombatCount{};

  /// Number of created records.
  uint32_t mNumCreatedRecords{};

  /// List of created base records. Includes spells, enchantments etc.
  /// explicitly created by the player as well as cloned objects.
  std::vector<oo::BaseId> mCreatedRecords{};

  /// Settings for the eight quick keys.
  /// \todo What is the meaning of the quick key data, are they formids?
  std::array<uint32_t, 8u> mQuickKeys{};

  /// Reticule data.
  /// \todo This needs to be decoded
  std::vector<uint8_t> mReticuleData{};

  /// Interface data.
  /// \todo This needs to be decoded.
  std::vector<uint8_t> mInterfaceData{};

  /// Region data.
  std::vector<Region> mRegions{};

  ///@}
};

std::istream &readSaveHeader(std::istream &is, SaveState &sv);

} // namespace oo

#endif // OPENOBL_SAVE_STATE_HPP
