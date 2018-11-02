#ifndef OPENOBLIVION_INITIAL_PROCESSOR_HPP
#define OPENOBLIVION_INITIAL_PROCESSOR_HPP

#include "esp_coordinator.hpp"
#include "record/io.hpp"
#include "records.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/interior_cell_resolver.hpp"
#include "resolvers/light_resolver.hpp"
#include "resolvers/static_resolver.hpp"

namespace engine {

class InitialProcessor {
 private:
  DoorResolver *doorRes;
  LightResolver *lightRes;
  StaticResolver *staticRes;
  InteriorCellResolver *interiorCellRes;

 public:
  InitialProcessor(DoorResolver *doorRes,
                   LightResolver *lightRes,
                   StaticResolver *staticRes,
                   InteriorCellResolver *interiorCellRes) :
      doorRes(doorRes),
      lightRes(lightRes),
      staticRes(staticRes),
      interiorCellRes(interiorCellRes) {}

  template<class R>
  void readRecord(esp::EspAccessor &accessor) {
    accessor.skipRecord();
  }

  template<>
  void readRecord<record::STAT>(esp::EspAccessor &accessor);

  template<>
  void readRecord<record::DOOR>(esp::EspAccessor &accessor);

  template<>
  void readRecord<record::LIGH>(esp::EspAccessor &accessor);

  template<>
  void readRecord<record::MISC>(esp::EspAccessor &accessor);

  template<>
  void readRecord<record::CELL>(esp::EspAccessor &accessor);

  template<>
  void readRecord<record::GMST>(esp::EspAccessor &accessor);
};

} // namespace engine

#endif // OPENOBLIVION_INITIAL_PROCESSOR_HPP
