#ifndef OPENOBLIVION_INTERIOR_CELL_RESOLVER_HPP
#define OPENOBLIVION_INTERIOR_CELL_RESOLVER_HPP

#include "bullet/configuration.hpp"
#include "keep_strategy.hpp"
#include "esp_coordinator.hpp"
#include "record/formid.hpp"
#include "record/record_header.hpp"
#include "record/records.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/light_resolver.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/static_resolver.hpp"
#include <btBulletDynamicsCommon.h>
#include <OgreColourValue.h>
#include <OgreLight.h>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <memory>
#include <string>
#include <unordered_map>

struct InteriorCell {
  std::string name{};
  Ogre::ColourValue ambientLight{};
  Ogre::Light *directionalLight{};
  gsl::not_null<gsl::owner<Ogre::SceneManager *>> scnMgr;
  std::unique_ptr<btDiscreteDynamicsWorld> physicsWorld{};

  explicit InteriorCell(std::unique_ptr<btDiscreteDynamicsWorld> physicsWorld)
      : scnMgr(Ogre::Root::getSingletonPtr()->createSceneManager()),
        physicsWorld(std::move(physicsWorld)) {}

  InteriorCell(const InteriorCell &other) = delete;
  InteriorCell &operator=(const InteriorCell &other) = delete;
  InteriorCell(InteriorCell &&other) = default;
  InteriorCell &operator=(InteriorCell &&other) = default;

  ~InteriorCell();
};

using InteriorCellResolver = Resolver<record::CELL>;

// We want the cell resolver to be able to decide to keep some cells loaded if
// they are accessed frequently, or have just been accessed, etc. This means the
// resolver must have sole or shared ownership of the cells. Since it is
// possible for NPCs to navigate through cells and follow the player, the AI
// code needs to be able to force cells to remain (at least partially) loaded.
// Thus we cannot allow loading a new cell to unconditionally delete an old one;
// it may still be in use. We therefore require shared ownership.
template<>
class Resolver<record::CELL> {
 private:
  class Entry {
   public:
    esp::EspAccessor mAccessor;
    std::unique_ptr<record::CELL> mRecord{};
    mutable std::weak_ptr<InteriorCell> mCell{};

    explicit Entry(esp::EspAccessor accessor);

    ~Entry() = default;

    Entry(const Entry &) = delete;
    Entry &operator=(const Entry &) = delete;

    Entry(Entry &&other) noexcept = default;
    Entry &operator=(Entry &&other) noexcept = default;
  };

  using Strategy = strategy::KeepStrategy<InteriorCell>;
  using Resolvers = std::tuple<Resolver<record::DOOR> &,
                               Resolver<record::LIGH> &,
                               Resolver<record::STAT> &>;
  Resolvers mResolvers;
  bullet::Configuration &mBulletConf;
  std::unordered_map<BaseId, Entry> mMap{};
  std::unique_ptr<Strategy> mStrategy;

 public:
  using store_t = Entry;
  using peek_t = record::CELL *;
  using get_t = record::CELL *;
  using make_t = std::shared_ptr<InteriorCell>;
  using resolvers_t = Resolvers;

  explicit Resolver(resolvers_t resolvers,
                    bullet::Configuration &bulletConf,
                    std::unique_ptr<Strategy> strategy) :
      mResolvers(std::move(resolvers)),
      mBulletConf(bulletConf),
      mStrategy(std::move(strategy)) {}

  peek_t peek(BaseId baseId) const;
  get_t get(BaseId baseId) const;
  make_t make(BaseId baseId) const;
  bool add(BaseId baseId, store_t entry);
};

class CellRecordVisitor {
 private:
  InteriorCell &mCell;
  Resolver<record::CELL>::resolvers_t mResolvers;
 public:
  explicit CellRecordVisitor(InteriorCell &cell,
                             Resolver<record::CELL>::resolvers_t resolvers) :
      mCell(cell), mResolvers(std::move(resolvers)) {}

  void setNodeTransform(Ogre::SceneNode *node,
                        const record::raw::REFRTransformation &transform);

  template<class R>
  void readRecord(esp::EspAccessor &accessor) {
    accessor.skipRecord();
  }

  template<>
  void readRecord<record::REFR>(esp::EspAccessor &accessor);

  template<class Refr, class ...Res>
  void readAndAttach(esp::EspAccessor &accessor,
                     gsl::not_null<Ogre::SceneNode *> node,
                     std::tuple<const Res &...> resolvers) {
    auto scnMgr{mCell.scnMgr};
    gsl::not_null<btDiscreteDynamicsWorld *> world{mCell.physicsWorld.get()};

    const auto ref{accessor.readRecord<Refr>().value};
    auto entity{reifyRecord(ref, scnMgr, std::move(resolvers))};
    setNodeTransform(node, ref);
    attachAll(node, RefId{ref.mFormId}, world, entity);
  }
};

#endif // OPENOBLIVION_INTERIOR_CELL_RESOLVER_HPP
