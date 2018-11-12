#ifndef OPENOBLIVION_INTERIOR_CELL_RESOLVER_HPP
#define OPENOBLIVION_INTERIOR_CELL_RESOLVER_HPP

#include "bullet/configuration.hpp"
#include "keep_strategy.hpp"
#include "esp_coordinator.hpp"
#include "formid.hpp"
#include "record/record_header.hpp"
#include "records.hpp"
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

    explicit Entry(esp::EspAccessor accessor) : mAccessor(accessor) {
      // Read from a copy to keep mAccessor pointing to the start of the record.
      esp::EspAccessor localAccessor{mAccessor};
      mRecord = std::make_unique<record::CELL>(
          localAccessor.readRecord<record::CELL>().value);
    }

    ~Entry() = default;

    Entry(const Entry &) = delete;
    Entry &operator=(const Entry &) = delete;

    Entry(Entry &&other) noexcept = default;
    Entry &operator=(Entry &&other) noexcept = default;
  };

  using Strategy = strategy::KeepStrategy<InteriorCell>;

  class Resolvers {
   public:
    Resolver<record::DOOR> &doorRes;
    Resolver<record::LIGH> &lighRes;
    Resolver<record::STAT> &statRes;

    Resolvers(Resolver<record::DOOR> &doorRes,
              Resolver<record::LIGH> &lighRes,
              Resolver<record::STAT> &statRes) :
        doorRes(doorRes), lighRes(lighRes), statRes(statRes) {}
  };

  class RecordVisitor {
   private:
    InteriorCell &mCell;
    Resolvers mResolvers;
   public:
    explicit RecordVisitor(InteriorCell &cell, Resolvers resolvers) :
        mCell(cell), mResolvers(resolvers) {}

    template<class R>
    void readRecord(esp::EspAccessor &accessor) {
      accessor.skipRecord();
    }

    template<>
    void readRecord<record::REFR>(esp::EspAccessor &accessor);
  };

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

  explicit Resolver(Resolvers resolvers,
                    bullet::Configuration &bulletConf,
                    std::unique_ptr<Strategy> strategy) :
      mResolvers(resolvers),
      mBulletConf(bulletConf),
      mStrategy(std::move(strategy)) {}

  peek_t peek(BaseId baseId) const;
  get_t get(BaseId baseId) const;
  make_t make(BaseId baseId) const;
  bool add(BaseId baseId, store_t entry);
};

#endif // OPENOBLIVION_INTERIOR_CELL_RESOLVER_HPP
