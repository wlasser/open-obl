#include "globals.hpp"
#include "record/records.hpp"
#include "settings.hpp"
#include "time_manager.hpp"
#include <spdlog/spdlog.h>
#include <memory>

namespace oo {

Globals::Globals() {
//  mValues["GameEra"].emplace<int16_t>(3);
//  mValues["GameYear"].emplace<int16_t>(433);
//  mValues["GameMonth"].emplace<int16_t>(unsigned(chrono::LastSeed));
//  mValues["GameDay"].emplace<int16_t>(25); // 26th
}

Globals &Globals::getSingleton() {
  // Cannot std::make_unique on a private constructor
  // TODO: Abseil TOTW #134
  static std::unique_ptr<Globals> instance{new Globals()};
  return *instance;
}

void Globals::load(const record::GLOB &rec, bool overwrite) {
  std::string key{rec.editorId.data};

  switch (rec.type.data) {
    case 's':
      // C++20: if (overwrite || !mValues.contains(key)) {
      if (overwrite || mValues.count(key) == 0) {
        mValues[key].emplace<int16_t>(rec.value.data);
      }
      break;
    case 'l':
      // C++20: if (overwrite || !mValues.contains(key)) {
      if (overwrite || mValues.count(key) == 0) {
        mValues[key].emplace<int32_t>(rec.value.data);
      }
      break;
    case 'f':
      // C++20: if (overwrite || !mValues.contains(keys)) {
      if (overwrite || mValues.count(key) == 0) {
        mValues[key].emplace<float>(rec.value.data);
      }
      break;
    default:
      spdlog::get(oo::LOG)->warn("GLOB {} has invalid type '{0:#x}'",
                                 key, rec.type.data);
      break;
  }
}

int16_t Globals::sGet(const std::string &edid) const {
  return get<int16_t>(edid).value();
}

int32_t Globals::lGet(const std::string &edid) const {
  return get<int32_t>(edid).value();
}

float Globals::fGet(const std::string &edid) const {
  return get<float>(edid).value();
}

int16_t &Globals::sGet(const std::string &edid) {
  return get<int16_t>(edid).value();
}

int32_t &Globals::lGet(const std::string &edid) {
  return get<int32_t>(edid).value();
}

float &Globals::fGet(const std::string &edid) {
  return get<float>(edid).value();
}

} // namespace oo