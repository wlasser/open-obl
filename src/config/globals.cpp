#include "config/globals.hpp"
#include "record/records.hpp"
#include "util/settings.hpp"
#include <spdlog/spdlog.h>
#include <memory>

namespace oo {

Globals::Globals() {
  mValues["GameEra"].emplace<int16_t>(0);
  mValues["GameYear"].emplace<int16_t>(0);
  mValues["GameMonth"].emplace<int16_t>(0);
  mValues["GameDay"].emplace<int16_t>(0);
  mValues["GameHour"].emplace<float>(0.0f);
  mValues["TimeScale"].emplace<int16_t>(30);
  mValues["GameDaysPassed"].emplace<int16_t>(0);
}

Globals &Globals::getSingleton() {
  // Cannot std::make_unique on a private constructor
  // TODO: Abseil TOTW #134
  static std::unique_ptr<Globals> instance{new Globals()};
  return *instance;
}

void Globals::load(const record::GLOB &rec, bool overwrite) {
  std::string key{rec.editorId.data};

  //C++20: if (mValues.contains(key)) {
  if (mValues.count(key) == 0) {
    switch (rec.type.data) {
      case 's':mValues[key].emplace<int16_t>(rec.value.data);
        break;
      case 'l':mValues[key].emplace<int32_t>(rec.value.data);
        break;
      case 'f':mValues[key].emplace<float>(rec.value.data);
        break;
      default:
        spdlog::get(oo::LOG)->warn("GLOB {} has invalid type '{0:#x}'",
                                   key, rec.type.data);
        break;
    }
  } else if (overwrite) {
    if (std::holds_alternative<int16_t>(mValues[key])) {
      mValues[key].emplace<int16_t>(static_cast<int16_t>(rec.value.data));
    } else if (std::holds_alternative<int32_t>(mValues[key])) {
      mValues[key].emplace<int32_t>(static_cast<int32_t>(rec.value.data));
    } else {
      mValues[key].emplace<float>(static_cast<float>(rec.value.data));
    }
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