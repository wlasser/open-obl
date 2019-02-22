#include "time_manager.hpp"
#include <memory>

namespace oo {

TimeManager &TimeManager::getSingleton() {
  // Cannot std::make_unique on a private constructor
  // TODO: Abseil TOTW #134
  static std::unique_ptr<TimeManager> instance{new TimeManager()};
  return *instance;
}

namespace chrono {

uint64_t GameClock::ticks = 0;

} // namespace chrono

} // namespace oo