#ifndef OPENOBLIVION_ENGINE_INTERIOR_CELL_RESOLVER_HPP
#define OPENOBLIVION_ENGINE_INTERIOR_CELL_RESOLVER_HPP

#include "bullet/configuration.hpp"
#include "engine/keep_strategy.hpp"
#include "engine/resolvers/light_resolver.hpp"
#include "engine/resolvers/static_resolver.hpp"
#include "formid.hpp"
#include "record/record_header.hpp"
#include "records.hpp"
#include <btBulletDynamicsCommon.h>
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
  std::unique_ptr<btDiscreteDynamicsWorld> physicsWorld{};

  explicit InteriorCell(std::unique_ptr<btDiscreteDynamicsWorld> physicsWorld)
      : scnMgr(Ogre::Root::getSingletonPtr()->createSceneManager()),
        physicsWorld(std::move(physicsWorld)) {}

  InteriorCell(const InteriorCell &other) = delete;
  InteriorCell &operator=(const InteriorCell &other) = delete;
  InteriorCell(InteriorCell &&other) = default;
  InteriorCell &operator=(InteriorCell &&other) = default;

  ~InteriorCell() {
    auto root = Ogre::Root::getSingletonPtr();
    if (root) root->destroySceneManager(scnMgr);
    for (int i = physicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
      btCollisionObject *obj = physicsWorld->getCollisionObjectArray()[i];
      physicsWorld->removeCollisionObject(obj);
    }
  }
};

struct InteriorCellEntry {
  long tell{};
  std::unique_ptr<record::CELL> record{};
  mutable std::weak_ptr<InteriorCell> cell{};
  // TODO: Constructors?
};

// We want the cell resolver to be able to decide to keep some cells loaded if
// they are accessed frequently, or have just been accessed, etc. This means the
// resolver must have sole or shared ownership of the cells. Since it is
// possible for NPCs to navigate through cells and follow the player, the AI
// code needs to be able to force cells to remain (at least partially) loaded.
// Thus we cannot allow loading a new cell to unconditionally delete an old one;
// it may still be in use. We therefore require shared ownership.
class InteriorCellResolver {
 public:
  using Strategy = strategy::KeepStrategy<InteriorCell>;

 private:
  class Processor {
   private:
    InteriorCell *cell;
    LightResolver *lightRes;
    StaticResolver *staticRes;

   public:
    explicit Processor(InteriorCell *cell,
                       LightResolver *lightRes,
                       StaticResolver *staticRes) :
        cell(cell),
        lightRes(lightRes),
        staticRes(staticRes) {}

    template<class R>
    void readRecord(std::istream &is) {
      record::skipRecord(is);
    }
  };

  std::istream &is;
  LightResolver *lightRes;
  StaticResolver *staticRes;
  bullet::Configuration *bulletConf;
  std::unordered_map<FormID, InteriorCellEntry> cells{};
  std::unique_ptr<Strategy> strategy;
  friend class InitialProcessor;

 public:
  explicit InteriorCellResolver(std::istream &is,
                                LightResolver *lightRes,
                                StaticResolver *staticRes,
                                bullet::Configuration *bulletConf,
                                std::unique_ptr<Strategy> &&strategy) :
      is(is),
      lightRes(lightRes),
      staticRes(staticRes),
      bulletConf(bulletConf),
      strategy(std::move(strategy)) {}

  record::CELL *peek(FormID baseID) const;
  std::shared_ptr<InteriorCell> get(FormID baseID) const;
};

template<>
void InteriorCellResolver::Processor::readRecord<record::REFR>(std::istream &);

} // namespace engine

#endif // OPENOBLIVION_ENGINE_INTERIOR_CELL_RESOLVER_HPP
