#include "game_settings.hpp"
#include "engine/conversions.hpp"
#include "engine/initial_processor.hpp"
#include "fs/path.hpp"
#include "record/group.hpp"
#include "record/io.hpp"
#include "records.hpp"
#include <cctype>

namespace engine {

template<>
void InitialProcessor::readRecord<record::STAT>(std::istream &is) {
  const auto rec{record::readRecord<record::STAT>(is)};
  Resolver<record::STAT>::store_t entry{};
  // MODL records omit the "meshes/" folder
  fs::Path rawPath{rec.data.modelFilename.data};
  entry.modelFilename = (fs::Path{"meshes"} / rawPath).view();
  staticRes->add(BaseId{rec.id}, std::move(entry));
}

template<>
void InitialProcessor::readRecord<record::DOOR>(std::istream &is) {
  auto rec = record::readRecord<record::DOOR>(is);
  Resolver<record::DOOR>::store_t entry{};

  if (rec.data.modelFilename) {
    fs::Path rawPath{rec.data.modelFilename->data};
    entry.modelFilename = (fs::Path{"meshes"} / rawPath).view();
  }
  doorRes->add(BaseId{rec.id}, std::move(entry));
}

template<>
void InitialProcessor::readRecord<record::LIGH>(std::istream &is) {
  auto rec = record::readRecord<record::LIGH>(is);
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
void InitialProcessor::readRecord<record::MISC>(std::istream &is) {
  auto rec = record::readRecord<record::MISC>(is);
  Resolver<record::STAT>::store_t entry{};
  if (rec.data.modelFilename) {
    entry.modelFilename = "meshes/" +
        conversions::normalizePath(rec.data.modelFilename->data);
  }
  staticRes->add(BaseId{rec.id}, std::move(entry));
}

template<>
void InitialProcessor::readRecord<record::CELL>(std::istream &is) {
  Resolver<record::CELL>::store_t entry{};
  entry.tell = is.tellg();
  entry.record = std::make_unique<record::CELL>();
  record::readRecord(is, *entry.record, "CELL");
  const BaseId baseId{entry.record->id};
  interiorCellRes->add(baseId, std::move(entry));
  // Children will be loaded later, so if this cell has any then skip over them
  if (record::peekGroupType(is) == record::Group::GroupType::CellChildren) {
    record::skipGroup(is);
  }
}

template<>
void InitialProcessor::readRecord<record::GMST>(std::istream &is) {
  auto rec = record::readRecord<record::GMST>(is);
  GameSettings::getSingleton().load(rec, true);
}

} // namespace engine