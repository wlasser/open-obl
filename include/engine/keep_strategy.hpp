#ifndef OPENOBLIVION_ENGINE_KEEP_STRATEGY_HPP
#define OPENOBLIVION_ENGINE_KEEP_STRATEGY_HPP

#include <memory>

namespace engine::strategy {

// Base class for strategies used to determine how long to keep things in
// memory. This is virtual because we want to be able to change strategies at
// runtime (e.g. based on memory constraints), but is also generic as each
// strategy is used concretely for only a single type.
// For instance, InteriorCellManager needs a KeepStrategy for InteriorCell, but
// doesn't care which one.
template<class T>
class KeepStrategy {
 public:
  virtual ~KeepStrategy() = 0;
  virtual void notify(std::shared_ptr<T> next) = 0;
};
template<class T>
inline KeepStrategy<T>::~KeepStrategy() = default;

// Enforce that at least one T is loaded at all times, for instance the
// InteriorCell that the player is currently in.
template<class T>
class KeepCurrent : public KeepStrategy<T> {
  std::shared_ptr<T> current{};
 public:
  void notify(std::shared_ptr<T> next) override {
    current = std::move(next);
  }
};

} // namespace engine::strategy

#endif // OPENOBLIVION_ENGINE_KEEP_STRATEGY_HPP
