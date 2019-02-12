#ifndef OPENOBLIVION_INITIAL_RECORD_VISITOR_HPP
#define OPENOBLIVION_INITIAL_RECORD_VISITOR_HPP

#include "esp_coordinator.hpp"
#include "record/io.hpp"
#include "record/records.hpp"
#include "resolvers/resolvers.hpp"

namespace oo {

class InitialRecordVisitor {
 private:
  oo::BaseResolversRef resolvers;

 public:
  explicit InitialRecordVisitor(oo::BaseResolversRef resolvers);

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
  void readRecord<record::WRLD>(oo::EspAccessor &accessor);

  template<>
  void readRecord<record::NPC_>(oo::EspAccessor &accessor);

  template<>
  void readRecord<record::GMST>(oo::EspAccessor &accessor);

  template<>
  void readRecord<record::RACE>(oo::EspAccessor &accessor);
};

} // namespace oo

#endif // OPENOBLIVION_INITIAL_RECORD_VISITOR_HPP
