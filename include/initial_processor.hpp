#ifndef OPENOBLIVION_INITIAL_PROCESSOR_HPP
#define OPENOBLIVION_INITIAL_PROCESSOR_HPP

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
  void readRecord(std::istream &is) {
    record::skipRecord(is);
  }

  template<>
  void readRecord<record::STAT>(std::istream &is);

  template<>
  void readRecord<record::DOOR>(std::istream &is);

  template<>
  void readRecord<record::LIGH>(std::istream &is);

  template<>
  void readRecord<record::MISC>(std::istream &is);

  template<>
  void readRecord<record::CELL>(std::istream &is);

  template<>
  void readRecord<record::GMST>(std::istream &is);
};

} // namespace engine

#endif // OPENOBLIVION_INITIAL_PROCESSOR_HPP
