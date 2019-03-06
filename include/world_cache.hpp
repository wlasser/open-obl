#ifndef OPENOBLIVION_WORLD_CACHE_HPP
#define OPENOBLIVION_WORLD_CACHE_HPP

#include "record/formid.hpp"
#include <boost/circular_buffer.hpp>
#include <memory>

namespace oo {

class World;

class WorldCache {
 public:
  using WorldPtr = std::shared_ptr<World>;
  using WorldBuffer = boost::circular_buffer<WorldPtr>;

  using GetResult = WorldPtr;

 private:
  WorldBuffer mWorlds{};

 public:
  explicit WorldCache(std::size_t capacity) : mWorlds(capacity) {}

  const WorldBuffer &worlds() const;

  void push_back(const WorldPtr &world);

  GetResult get(oo::BaseId id) const;

  /// Move the given worldspace to the back of its buffer, if it exists.
  void promote(oo::BaseId id);
};

} // namespace oo

#endif //OPENOBLIVION_WORLD_CACHE_HPP
