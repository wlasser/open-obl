#include "conversions.hpp"
#include "esp_coordinator.hpp"
#include "fs/path.hpp"
#include "game_settings.hpp"
#include "initial_processor.hpp"
#include "record/group.hpp"
#include "record/io.hpp"
#include "records.hpp"
#include <cctype>

template<>
void InitialProcessor::readRecord<record::STAT>(esp::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::STAT>().value};
  Resolver<record::STAT>::store_t entry{};
  // MODL records omit the "meshes/" folder
  fs::Path rawPath{rec.data.modelFilename.data};
  entry.modelFilename = (fs::Path{"meshes"} / rawPath).view();
  staticRes->add(BaseId{rec.id}, std::move(entry));
}

template<>
void InitialProcessor::readRecord<record::DOOR>(esp::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::DOOR>().value};
  Resolver<record::DOOR>::store_t entry{};

  if (rec.data.modelFilename) {
    fs::Path rawPath{rec.data.modelFilename->data};
    entry.modelFilename = (fs::Path{"meshes"} / rawPath).view();
  }
  doorRes->add(BaseId{rec.id}, std::move(entry));
}

template<>
void InitialProcessor::readRecord<record::LIGH>(esp::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::LIGH>().value};
  using Flag = record::raw::DATA_LIGH::Flag;
  if (rec.data.data.data.flags & Flag::CanBeCarried) {
    // TODO: Support carriable lights
  } else {
    Resolver<record::LIGH>::store_t entry{};
    const auto &data = rec.data.data.data;

    if (rec.data.modelFilename) {
      entry.modelFilename = "meshes/" +
          conversions::normalizePath(rec.data.modelFilename->data);
    }
    if (rec.data.sound) entry.sound = rec.data.sound->data;
    if (rec.data.itemScript) entry.script = rec.data.itemScript->data;
    if (rec.data.fadeValue) entry.fadeValue = rec.data.fadeValue->data;
    entry.radius = static_cast<float>(data.radius);
    entry.falloffExponent = data.falloffExponent;
    entry.fov = data.fov;
    entry.color.setAsABGR(data.color.v);
    entry.flags = data.flags;

    lightRes->add(BaseId{rec.id}, std::move(entry));
  }
}

template<>
void InitialProcessor::readRecord<record::MISC>(esp::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::MISC>().value};
  Resolver<record::STAT>::store_t entry{};
  if (rec.data.modelFilename) {
    entry.modelFilename = "meshes/" +
        conversions::normalizePath(rec.data.modelFilename->data);
  }
  staticRes->add(BaseId{rec.id}, std::move(entry));
}

template<>
void InitialProcessor::readRecord<record::CELL>(esp::EspAccessor &accessor) {
  // Constructor takes accessor by *value* so it can be stored for deferred
  // loading. It therefore calls readRecord on the *copy*, so does not advance
  // our accessor.
  Resolver<record::CELL>::store_t entry(accessor);
  accessor.skipRecord();
  const BaseId baseId{entry.mRecord->id};
  interiorCellRes->add(baseId, std::move(entry));
  // Children will be loaded later, so if this cell has any then skip over them
  if (accessor.peekGroupType() == record::Group::GroupType::CellChildren) {
    accessor.skipGroup();
  }
}

template<>
void InitialProcessor::readRecord<record::GMST>(esp::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::GMST>().value};
  GameSettings::getSingleton().load(rec, true);
}
