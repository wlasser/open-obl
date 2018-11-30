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
  LighResolver *lighRes;
  StatResolver *statRes;
  ActiResolver *actiRes;
  CellResolver *cellRes;

 public:
  InitialRecordVisitor(DoorResolver *doorRes,
                       LighResolver *lighRes,
                       StatResolver *statRes,
                       ActiResolver *actiRes,
                       CellResolver *cellRes) :
      doorRes(doorRes),
      lighRes(lighRes),
      statRes(statRes),
      actiRes(actiRes),
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
