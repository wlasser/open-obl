#ifndef OPENOBLIVION_INITIAL_RECORD_VISITOR_HPP
#define OPENOBLIVION_INITIAL_RECORD_VISITOR_HPP

#include "esp_coordinator.hpp"
#include "record/io.hpp"
#include "record/records.hpp"
#include "resolvers/acti_resolver.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/light_resolver.hpp"
#include "resolvers/static_resolver.hpp"

class InitialRecordVisitor {
 private:
  DoorResolver *doorRes;
  LightResolver *lightRes;
  StaticResolver *staticRes;
  ActivatorResolver *activatorRes;
  RefrDoorResolver *refrDoorRes;
  RefrLightResolver *refrLightRes;
  RefrStaticResolver *refrStaticRes;
  RefrActivatorResolver *refrActivatorRes;
  CellResolver *cellRes;

 public:
  InitialRecordVisitor(DoorResolver *doorRes,
                       LightResolver *lightRes,
                       StaticResolver *staticRes,
                       ActivatorResolver *activatorRes,
                       RefrDoorResolver *refrDoorRes,
                       RefrLightResolver *refrLightRes,
                       RefrStaticResolver *refrStaticRes,
                       RefrActivatorResolver *refrActivatorRes,
                       CellResolver *cellRes) :
      doorRes(doorRes),
      lightRes(lightRes),
      staticRes(staticRes),
      activatorRes(activatorRes),
      refrDoorRes(refrDoorRes),
      refrLightRes(refrLightRes),
      refrStaticRes(refrStaticRes),
      refrActivatorRes(refrActivatorRes),
      cellRes(cellRes) {}

  template<class R>
  void readRecord(oo::EspAccessor &accessor) {
    accessor.skipRecord();
  }

  template<>
  void readRecord<record::ACTI>(oo::EspAccessor &accessor);

  template<>
  void readRecord<record::STAT>(oo::EspAccessor &accessor);

  template<>
  void readRecord<record::DOOR>(oo::EspAccessor &accessor);

  template<>
  void readRecord<record::LIGH>(oo::EspAccessor &accessor);

  template<>
  void readRecord<record::MISC>(oo::EspAccessor &accessor);

  template<>
  void readRecord<record::CELL>(oo::EspAccessor &accessor);

  template<>
  void readRecord<record::GMST>(oo::EspAccessor &accessor);
};

#endif // OPENOBLIVION_INITIAL_RECORD_VISITOR_HPP
