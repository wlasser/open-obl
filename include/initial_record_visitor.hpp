#ifndef OPENOBL_INITIAL_RECORD_VISITOR_HPP
#define OPENOBL_INITIAL_RECORD_VISITOR_HPP

#include "esp/esp_coordinator.hpp"
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
};

//===----------------------------------------------------------------------===//
// readRecord specializations.
// See GCC Bug 85282, CWG 727 DR
//===----------------------------------------------------------------------===//

template<> void
InitialRecordVisitor::readRecord<record::GMST>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::GLOB>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::RACE>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::LTEX>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::ACTI>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::CONT>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::DOOR>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::LIGH>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::MISC>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::STAT>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::GRAS>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::TREE>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::FLOR>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::FURN>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::NPC_>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::WTHR>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::CLMT>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::CELL>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::WRLD>(oo::EspAccessor &accessor);
template<> void
InitialRecordVisitor::readRecord<record::WATR>(oo::EspAccessor &accessor);

} // namespace oo

#endif // OPENOBL_INITIAL_RECORD_VISITOR_HPP
