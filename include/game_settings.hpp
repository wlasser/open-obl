#ifndef OPENOBLIVION_GAME_SETTINGS_HPP
#define OPENOBLIVION_GAME_SETTINGS_HPP

#include "record/records.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <filesystem>
#include <fstream>
#include <mutex>

class GameSettings {
 private:
  boost::property_tree::ptree tree{};
  GameSettings() = default;

 public:

  GameSettings(const GameSettings &other) = delete;
  GameSettings &operator=(const GameSettings &other) = delete;
  GameSettings(GameSettings &&other) = delete;
  GameSettings &operator=(GameSettings &&other) = delete;

  static GameSettings &getSingleton() {
    // Cannot std::make_unique on a private constructor
    // TODO: Abseil TOTW #134
    static std::unique_ptr<GameSettings> instance{new GameSettings()};
    return *instance;
  }

  // Load all the settings in an ini file, optionally overwriting any existing
  // values with new ones.
  void load(const std::filesystem::path &filename, bool /*overwrite = true*/) {
    // The game ini files has a duplicate key General.STestFile1 and a
    // multiline string GeneralWarnings.SMasterMismatchWarning, which are not
    // supported by the property_tree parser.
    // TODO: Replace ini parser with a custom parser (boost::spirit?)
    boost::property_tree::ini_parser::read_ini(filename, tree);
  }

  // Load the setting from a GMST record, optionally overwriting any existing
  // value with the new one.
  void load(const record::GMST &gmst, bool overwrite = true) {
    std::string key = gmst.editorId.data;
    if (key.empty()) return;
    // put overwrites without question, so we have to query existence first if
    // overwrite is not set.
    switch (key[0]) {
      case 'f':
        if (overwrite || !tree.get_optional<float>(key)) {
          tree.put<float>(key, gmst.value.data.f);
        }
        break;
      case 'i':
        if (overwrite || !tree.get_optional<int>(key)) {
          tree.put<int>(key, gmst.value.data.i);
        }
        break;
      case 's':
        if (overwrite || !tree.get_optional<std::string>(key)) {
          const auto &arr = gmst.value.data.s;
          std::string s(arr.begin(), arr.end());
          tree.put<std::string>(key, s);
        }
        break;
      default: break;
    }
  }

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

#endif //OPENOBLIVION_GAME_SETTINGS_HPP
