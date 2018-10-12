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
};

template<>
void InitialProcessor::readRecord<record::STAT>(std::istream &);

template<>
void InitialProcessor::readRecord<record::DOOR>(std::istream &);

template<>
void InitialProcessor::readRecord<record::LIGH>(std::istream &);

template<>
void InitialProcessor::readRecord<record::MISC>(std::istream &);

template<>
void InitialProcessor::readRecord<record::CELL>(std::istream &);

template<>
void InitialProcessor::readRecord<record::GMST>(std::istream &);

} // namespace engine

#endif // OPENOBLIVION_ENGINE_INITIAL_PROCESSOR_HPP
