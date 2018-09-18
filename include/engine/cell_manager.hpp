#ifndef OPENOBLIVION_ENGINE_CELL_MANAGER_HPP
#define OPENOBLIVION_ENGINE_CELL_MANAGER_HPP

#include "engine/light_manager.hpp"
#include "engine/static_manager.hpp"
#include "formid.hpp"
#include "record/record_header.hpp"
#include "records.hpp"
#include <OgreColourValue.h>
#include <OgreLight.h>
#include <OgreSceneManager.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace engine {

struct InteriorCell {
  std::string name{};
  Ogre::ColourValue ambientLight{};
  Ogre::Light *directionalLight{};
  Ogre::SceneManager *scnMgr{};
};

struct InteriorCellEntry {
  long tell{};
  std::unique_ptr<record::CELL> record{};
  std::weak_ptr<InteriorCell> cell{};
  // TODO: Constructors?
};

class InteriorCellManager {
 private:
  class Processor {
   private:
    InteriorCell *cell;
    LightManager *lightMgr;
    StaticManager *staticMgr;

   public:
    explicit Processor(InteriorCell *cell,
                       LightManager *lightMgr,
                       StaticManager *staticMgr) :
        cell(cell),
        lightMgr(lightMgr),
        staticMgr(staticMgr) {}

    template<class R>
    void readRecord(std::istream &is) {
      record::skipRecord(is);
    }
  };

  std::istream &is;
  LightManager *lightMgr;
  StaticManager *staticMgr;
  std::unordered_map<FormID, InteriorCellEntry> cells{};
  friend class InitialProcessor;

 public:
  explicit InteriorCellManager(std::istream &is,
                               LightManager *lightMgr,
                               StaticManager *staticMgr) :
      is(is),
      lightMgr(lightMgr),
      staticMgr(staticMgr) {}
  record::CELL *peek(FormID baseID);
  std::shared_ptr<InteriorCell> get(FormID baseID, Ogre::SceneManager *mgr);
};

template<>
void InteriorCellManager::Processor::readRecord<record::REFR>(std::istream &);

} // namespace engine

#endif // OPENOBLIVION_ENGINE_CELL_MANAGER_HPP
