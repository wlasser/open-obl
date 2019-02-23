#ifndef OPENOBLIVION_GLOBALS_HPP
#define OPENOBLIVION_GLOBALS_HPP

#include "record/records_fwd.hpp"
#include <tl/optional.hpp>
#include <map>
#include <string>
#include <variant>

namespace oo {

/// Manager of global variables introduced by the game data.
/// This is a global container of globals, and is intended to manage the various
/// `record::GLOB` records introduced by esp/esm files. Specifically, it differs
/// from `oo::GameSettings` in the mutability of its values; globals can be
/// changed at runtime and recorded in the player's save game, whereas game
/// settings are intended to be immutable.
class Globals {
 private:
  using Storage = std::variant<int16_t, int32_t, float>;

  std::map<std::string, Storage> mValues;

  Globals();

 public:
  Globals(const Globals &) = delete;
  Globals &operator=(const Globals &) = delete;
  Globals(Globals &&) = delete;
  Globals &operator=(Globals &&) = delete;

  static Globals &getSingleton();

  /// Load the setting from a `record::GLOB`, optionally overwriting any
  /// existing value with the new one
  /// \warning The type of a global will not be changed, even if `overwrite` is
  ///          true. This is mostly because `GameHour` is incorrectly typed in
  ///          `oblivion.esm` as a `short` instead of a `float`.
  void load(const record::GLOB &rec, bool overwrite = true);

  template<class T> tl::optional<T> get(const std::string &edid) const;
  template<class T> T get(const std::string &edid, T defaultValue) const;

  template<class T> tl::optional<T &> get(const std::string &edid);

  int16_t sGet(const std::string &edid) const;
  int32_t lGet(const std::string &edid) const;
  float fGet(const std::string &edid) const;

  int16_t &sGet(const std::string &edid);
  int32_t &lGet(const std::string &edid);
  float &fGet(const std::string &edid);
};

//===----------------------------------------------------------------------===//
// Globals member function implementations
//===----------------------------------------------------------------------===//

template<class T>
tl::optional<T> Globals::get(const std::string &edid) const {
  static_assert(std::is_same_v<T, int16_t>
                    || std::is_same_v<T, int32_t>
                    || std::is_same_v<T, float>,
                "GLOB type must be one of int16_t, int32_t, or float");
  auto it{mValues.find(edid)};
  if (it == mValues.end()) return tl::nullopt;

  auto *ptr{std::get_if<T>(&it->second)};
  return ptr ? *ptr : tl::optional<T>{};
}

template<class T>
T Globals::get(const std::string &edid, T defaultValue) const {
  return get<T>(edid).value_or(defaultValue);
}

template<class T> tl::optional<T &> Globals::get(const std::string &edid) {
  static_assert(std::is_same_v<T, int16_t>
                    || std::is_same_v<T, int32_t>
                    || std::is_same_v<T, float>,
                "GLOB type must be one of int16_t, int32_t, or float");
  auto it{mValues.find(edid)};
  if (it == mValues.end()) return tl::nullopt;

  auto *ptr{std::get_if<T>(&it->second)};
  return ptr ? *ptr : tl::optional<T &>{};
}

} // namespace oo

#endif // OPENOBLIVION_GLOBALS_HPP
