#ifndef OPENOBLIVION_GAME_SETTINGS_HPP
#define OPENOBLIVION_GAME_SETTINGS_HPP

#include "fs/path.hpp"
#include "record/records_fwd.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <mutex>
#include <string>

namespace oo {

/// Manager of immutable global variables introduced by the game data and
/// configuration files.
/// This is a global container of globals, and is used to manage the various
/// `record::GMST` records introduced by esp/esm files, as well as the INI
/// configuration files used by the game. Specifically, it differs from
/// `oo::Globals` in the immutability of its values; game settings are
/// intended to be immutable, whereas globals can be changed at runtime and
/// record in the player's save game.
///
/// Settings are grouped into sections which precede the setting name and are
/// separated by it by a `.`, such as `Foo.barSetting`, which describes a
/// setting `barSetting` in the section `Foo`. All settings defined in an INI
/// file must belong to exactly one section, whereas it is expected that those
/// defined by a `record::GMST` record do not have a section, and instead are
/// referred to by just their name.
///
/// Each setting is one of several types indicated by a single character prefix
/// to the setting's name. The types and their prefixes are as follows.
///
/// C++ Type      | Prefix Character
/// ------------- | ----------------
/// `bool`        | `b`
/// `float`       | `f`
/// `unsigned`    | `u`
/// `int`         | `i`
/// `std::string` | `s`
///
/// The following table describes the INI-defined `oo::GameSetting`s used by the
/// program.
///
/// <table>
/// <tr><th>Setting Group and Name</th><th>Setting Description</th></tr>
/// <tr><td>General.SLocalMasterPath</td>
///     <td>The name of the directory containing the BSA and esp/esm files,
///         relative to the location of the executable.</td></tr>
/// <tr><td>General.uGridsToLoad</td>
///     <td>The diameter of cells to load at full detail around the player.
///         Defines the size of the player's *near neighbourhood*.
///         Should be a positive odd integer.</td></tr>
/// <tr><td>General.uGridDistantCount</td>
///     <td>The diameter of cells to load at low detail around the player.
///         Defines the size of the player's *far neighbourhood*.
///         Should be a positive odd integer, and greater than or equal to
///         `General.uGridsToLoad`.</td></tr>
/// <tr><td>Archive.sArchiveList</td>
///     <td>A comma-separated list of BSA files to load, relative to
///         `General.SLocalMasterPath`.</td></tr>
/// <tr><td>Debug.sOgreLogLevel</td>
///     <td>The minimum level of log message issued by the OGRE logger that will
///         appear in the log. Specifically, must be a string accepted by
///         `spdlog::from_str()`.</td></tr>
/// <tr><td>Debug.sLogLevel</td>
///     <td>The minimum level of log message issued by the OO logger that will
///         appear in the log. Specifically, must be a string accepted by
///         `spdlog::from_str()`.</td></tr>
/// <tr><td>Display.iSize W</td>
///     <td>The width of the game window in pixels. Must be positive.</td></tr>
/// <tr><td>Display.iSize H</td>
///     <td>The height of the game window in pixels. Must be positive.</td></tr>
/// <tr><td>Display.bFull Screen</td>
///     <td>Whether the game should be displayed in full-screen mode.</td></tr>
/// <tr><td>Audio.fDefaultMasterVolume</td>
///     <td>The volume of the master audio bus. Should be between 0 and 1.
///         </td></tr>
/// <tr><td>Audio.fDefaultMusicVolume</td>
///     <td>The volume of the music audio bus. Should be between 0 and 1.
///         </td></tr>
/// <tr><td>Audio.fDefaultEffectsVolume</td>
///     <td>The volume of the effects audio bus. Should be between 0 and 1.
///         </td></tr>
/// <tr><td>Audio.fDefaultFootVolume</td>
///     <td>The volume of the foot audio bus. Should be between 0 and 1.
///         </td></tr>
/// <tr><td>Audio.fDefaultVoiceVolume</td>
///     <td>The volume of the voice audio bus. Should be between 0 and 1.
///         </td></tr>
/// <tr><td>LOD.iLODTextureSizePow2</td>
///     <td>The power-of-two defining the size in pixels of the baked LOD
///         textures used to render mid-distance terrain. Specifically the size
///         of each quad's (one quarter of a cell) LOD texture is two raised to
///         the power of this setting. Should be between 1 and 16.
///
///         Terrain in the player's far neighbourhood is
///         rendered at a lower level of detail and uses a baked diffuse map
///         instead of blending the ground terrain layers together at runtime.
///         The baked diffuse maps are created at runtime and can be quite
///         memory intensive, since each cell has four of them; one for each
///         quadrant. It is recommended that this value is kept low---say below
///         10---unless `General.uGridDistantCount` is small.</td></tr>
/// <tr><td>bLightAttenuation.fLinearRadiusMult</td>
///     <td>Multiplier to apply to the light radius in the linear part of the
///         point light attenuation equation.</td></tr>
/// <tr><td>bLightAttenuation.fQuadraticRadiusMult</tr>
///     <td>Multiplier to apply to the light radius in the quadratic part of the
///         point light attenuation equation.</td></tr>
/// <tr><td>bLightAttenuation.fConstantValue</td>
///     <td>Coefficient of the constant term in the point light attenuation
///         equation.</td></tr>
/// <tr><td>bLightAttenuation.fLinearValue</td>
///     <td>Coefficient of the linear term in the point light attenuation
///         equation.</td></tr>
/// <tr><td>bLightAttenuation.fQuadraticValue</td>
///     <td>Coefficient of the quadratic term in the point light attenuation
///         equation.</td></tr>
/// <tr><td>Controls.fMouseSensitivity</td>
///     <td>Conversion factor from a mouse move delta in pixels to a change in
///         camera look angle in radians.</td></tr>
/// <tr><td>Fonts.SFontFile_X</td>
///     <td>Path to the font file describing font number `X` relative to
///         `General.SLocalMasterPath`. This option can appear any number of
///         times with `X` replaced by a different positive integer each time.
///         It is expected that settings appear at least for `X` equal to 1
///         through 5.
///
///         Note: The original implementation used paths relative to the
///         application, not relative to `General.SLocalMasterPath`.</td></tr>
/// </table>
///
class GameSettings {
 private:
  boost::property_tree::ptree tree{};
  GameSettings() = default;

 public:

  GameSettings(const GameSettings &other) = delete;
  GameSettings &operator=(const GameSettings &other) = delete;
  GameSettings(GameSettings &&other) = delete;
  GameSettings &operator=(GameSettings &&other) = delete;

  static GameSettings &getSingleton();

  /// Load all the settings in an INI file, optionally overwriting any existing
  /// values with new ones.
  void load(const char *filename, bool overwrite = true);

  /// Load the setting from a `record::GMST` record, optionally overwriting any
  /// existing value with the new one.
  void load(const record::GMST &gmst, bool overwrite = true);

  template<class T>
  boost::optional<T> get(const std::string &path) const {
    return tree.get_optional<T>(path);
  }

  template<class T>
  T get(const std::string &path, const T &defaultValue) const {
    return tree.get<T>(path, defaultValue);
  }

  std::string get(const std::string &path, const char *defaultValue) const {
    return tree.get<std::string>(path, defaultValue);
  }

  bool bGet(const std::string &path) const {
    return get<bool>(path).value();
  }

  float fGet(const std::string &path) const {
    return get<float>(path).value();
  }

  int iGet(const std::string &path) const {
    return get<int>(path).value();
  }

  std::string sGet(const std::string &path) const {
    return get<std::string>(path).value();
  }

  unsigned int uGet(const std::string &path) const {
    return get<unsigned int>(path).value();
  }

  /// Convenience function to return the font with the given index.
  /// Returns the value of `Fonts.sFontFile_X` with `X` replaced by the decimal
  /// value of `index`, or a default font if no such key exists.
  oo::Path getFont(int index) const;
};

template<class T>
class GameSetting {
 private:
  mutable T value;
  mutable bool loaded{false};
  mutable std::mutex mutables{};
  /*const*/ std::string path;

 public:
  explicit GameSetting(std::string path, const T &defaultValue = {})
      : value(defaultValue), path(std::move(path)) {}

  GameSetting(const GameSetting &other) = default;
  GameSetting &operator=(const GameSetting &other) = default;
  GameSetting(GameSetting &&other) = delete;
  GameSetting &operator=(GameSetting &&other) = delete;

  // Return the value of the setting.
  // If the value has not yet been loaded successfully, this attempts to load
  // it. If loading fails, the default is returned. If loading succeeds, the
  // value is cached and any subsequent calls will return that value without a
  // load.
  T get() const noexcept {
    if (!loaded) {
      std::unique_lock lock{mutables};
      auto opt = GameSettings::getSingleton().get<T>(path);
      if (opt) {
        value = *opt;
        loaded = true;
      }
      loaded = true;
    }
    return value;
  }

  T operator*() const noexcept {
    return get();
  }

  explicit operator T() const noexcept {
    return get();
  }
};

} // namespace oo

#endif //OPENOBLIVION_GAME_SETTINGS_HPP
