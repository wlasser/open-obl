#ifndef OPENOBLIVION_ENGINE_INITIAL_PROCESSOR_HPP
#define OPENOBLIVION_ENGINE_INITIAL_PROCESSOR_HPP

#include "engine/resolvers/door_resolver.hpp"
#include "engine/resolvers/interior_cell_resolver.hpp"
#include "engine/resolvers/light_resolver.hpp"
#include "engine/resolvers/static_resolver.hpp"
#include "record/io.hpp"
#include "records.hpp"

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

#endif // OPENOBLIVION_ENGINE_INITIAL_PROCESSOR_HPP
