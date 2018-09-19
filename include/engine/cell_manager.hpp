#ifndef OPENOBLIVION_ENGINE_CELL_MANAGER_HPP
#define OPENOBLIVION_ENGINE_CELL_MANAGER_HPP

#include "engine/keep_strategy.hpp"
#include "engine/light_manager.hpp"
#include "engine/static_manager.hpp"
#include "formid.hpp"
#include "record/record_header.hpp"
#include "records.hpp"
#include <OgreColourValue.h>
#include <OgreLight.h>
#include <OgreRoot.h>
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
  InteriorCell() : scnMgr(Ogre::Root::getSingletonPtr()
                              ->createSceneManager()) {}
  ~InteriorCell() {
    auto root = Ogre::Root::getSingletonPtr();
    if (root) root->destroySceneManager(scnMgr);
  }
};

struct InteriorCellEntry {
  long tell{};
  std::unique_ptr<record::CELL> record{};
  std::weak_ptr<InteriorCell> cell{};
  // TODO: Constructors?
};

// We want the cell manager to be able to decide to keep some cells loaded if
// they are accessed frequently, or have just been accessed, etc. This means the
// manager must have sole or shared ownership of the cells. Since it is possible
// for NPCs to navigate through cells and follow the player, the AI code in
// particular needs to be able to force cells to remain (at least partially)
// loaded. Thus we cannot allow loading a new cell to unconditionally delete an
// old one; it may still be in use. We therefore require shared ownership.
class InteriorCellManager {
 public:
  using Strategy = strategy::KeepStrategy<InteriorCell>;

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
  std::unique_ptr<Strategy> strategy;
  friend class InitialProcessor;

 public:
  explicit InteriorCellManager(std::istream &is,
                               LightManager *lightMgr,
                               StaticManager *staticMgr,
                               std::unique_ptr<Strategy> &&strategy) :
      is(is),
      lightMgr(lightMgr),
      staticMgr(staticMgr),
      strategy(std::move(strategy)) {}

  record::CELL *peek(FormID baseID);
  std::shared_ptr<InteriorCell> get(FormID baseID);
};

template<>
void InteriorCellManager::Processor::readRecord<record::REFR>(std::istream &);

} // namespace engine

#endif // OPENOBLIVION_ENGINE_CELL_MANAGER_HPP
