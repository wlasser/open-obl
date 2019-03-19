#ifndef OPENOBLIVION_INITIAL_RECORD_VISITOR_HPP
#define OPENOBLIVION_INITIAL_RECORD_VISITOR_HPP

#include "esp_coordinator.hpp"
#include "persistent_reference_locator.hpp"
#include "record/io.hpp"
#include "record/records.hpp"
#include "resolvers/resolvers.hpp"

namespace oo {

class InitialRecordVisitor {
 private:
  oo::BaseResolversRef mBaseCtx;
  oo::RefrResolversRef mRefrCtx;
  PersistentReferenceLocator &mRefMap;

 public:
  explicit InitialRecordVisitor(oo::BaseResolversRef baseCtx,
                                oo::RefrResolversRef refrCtx,
                                PersistentReferenceLocator &refMap) noexcept;

  template<class R> void readRecord(oo::EspAccessor &accessor) {
    accessor.skipRecord();
  }

  template<> void readRecord<record::GMST>(oo::EspAccessor &accessor);
  template<> void readRecord<record::GLOB>(oo::EspAccessor &accessor);
  template<> void readRecord<record::RACE>(oo::EspAccessor &accessor);
  template<> void readRecord<record::LTEX>(oo::EspAccessor &accessor);
  template<> void readRecord<record::ACTI>(oo::EspAccessor &accessor);
  template<> void readRecord<record::DOOR>(oo::EspAccessor &accessor);
  template<> void readRecord<record::LIGH>(oo::EspAccessor &accessor);
  template<> void readRecord<record::MISC>(oo::EspAccessor &accessor);
  template<> void readRecord<record::STAT>(oo::EspAccessor &accessor);
  template<> void readRecord<record::GRAS>(oo::EspAccessor &accessor);
  template<> void readRecord<record::TREE>(oo::EspAccessor &accessor);
  template<> void readRecord<record::NPC_>(oo::EspAccessor &accessor);
  template<> void readRecord<record::WTHR>(oo::EspAccessor &accessor);
  template<> void readRecord<record::CLMT>(oo::EspAccessor &accessor);
  template<> void readRecord<record::CELL>(oo::EspAccessor &accessor);
  template<> void readRecord<record::WRLD>(oo::EspAccessor &accessor);
  template<> void readRecord<record::WATR>(oo::EspAccessor &accessor);
};

} // namespace oo

#endif // OPENOBLIVION_INITIAL_RECORD_VISITOR_HPP
