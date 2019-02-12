#ifndef OPENOBLIVION_WRLD_RESOLVER_HPP
#define OPENOBLIVION_WRLD_RESOLVER_HPP

#include "esp_coordinator.hpp"
#include "resolvers/resolvers.hpp"
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <tl/optional.hpp>
#include <utility>

namespace oo {

template<>
class Resolver<record::WRLD> {
 private:
  struct Metadata {
    /// Accessors, in load order of mods that modify the contents of the world.
    std::vector<oo::EspAccessor> mAccessors{};
    /// All cells in the world.
    absl::flat_hash_set<oo::BaseId> mCells{};
  };

  /// Holds a record with an immutable backup of the original.
  /// Unlike general record::Record, it is not possible to create a new
  /// record::WRLD at runtime.
  using RecordEntry = std::pair<record::WRLD, tl::optional<record::WRLD>>;
  using WrappedRecordEntry = std::pair<RecordEntry, Metadata>;

  /// Record storage.
  absl::flat_hash_map<oo::BaseId, WrappedRecordEntry> mRecords{};
  class WrldVisitor;
 public:
  using RecordIterator = typename decltype(mRecords)::iterator;

  /// Insert a new record with the given accessor if one exists, otherwise
  /// replace the existing record and append the accessor to the accessor list.
  std::pair<RecordIterator, bool>
  insertOrAppend(oo::BaseId baseId,
                 const record::WRLD &rec,
                 oo::EspAccessor accessor);

  /// Return a reference to the world.
  tl::optional<const record::WRLD &> get(oo::BaseId baseId) const;

  /// \overload get(oo::BaseId)
  tl::optional<record::WRLD &> get(oo::BaseId baseId);

  /// Check if there is a world with the baseId.
  bool contains(oo::BaseId baseId) const;

  using BaseResolverContext = std::tuple<Resolver<record::CELL> &>;

  /// Register all cell children of the world.
  void load(oo::BaseId baseId, BaseResolverContext baseCtx);

  /// Return the BaseIds of all cells in the world.
  /// \warning This will return an empty optional if the world has not been
  ///          loaded first with a call to load.
  tl::optional<const absl::flat_hash_set<BaseId> &>
  getCells(oo::BaseId baseId) const;
};

class Resolver<record::WRLD>::WrldVisitor {
 public:
  using Metadata = Resolver<record::WRLD>::Metadata;
  using BaseContext = Resolver<record::WRLD>::BaseResolverContext;

 private:
  Metadata &mMeta;
  BaseContext mBaseCtx;

 public:
  WrldVisitor(Metadata &meta, BaseContext baseCtx)
      : mMeta(meta), mBaseCtx(std::move(baseCtx)) {}

  template<class R> void readRecord(oo::EspAccessor &accessor) {
    accessor.skipRecord();
  }

  template<> void readRecord<record::CELL>(oo::EspAccessor &accessor);
  // TODO: record::ROAD specialization
};

class World {
 public:
  std::string name{};
  oo::BaseId baseId{};
  gsl::not_null<gsl::owner<Ogre::SceneManager *>> scnMgr;

  explicit World(oo::BaseId baseId)
      : baseId(baseId),
        scnMgr(Ogre::Root::getSingleton().createSceneManager()) {}

  ~World();
  World(const World &) = delete;
  World &operator=(const World &) = delete;
  World(World &&) noexcept = default;
  World &operator=(World &&) noexcept = default;
};

template<>
struct ReifyRecordTrait<record::WRLD> {
  using type = std::shared_ptr<World>;
  using resolvers = decltype(std::tuple_cat(
      std::declval<Resolver<record::WRLD>::BaseResolverContext>(),
      std::declval<std::tuple<const oo::Resolver<record::WRLD> &>>()));
};

/// Not a specialization because passing an Ogre::SceneManager doesn't make
/// sense.
ReifyRecordTrait<record::WRLD>::type
reifyRecord(const record::WRLD &refRec,
            ReifyRecordTrait<record::WRLD>::resolvers resolvers);

} // namespace oo

#endif // OPENOBLIVION_WRLD_RESOLVER_HPP
