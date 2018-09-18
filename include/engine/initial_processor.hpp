#ifndef OPENOBLIVION_ENGINE_INITIAL_PROCESSOR_HPP
#define OPENOBLIVION_ENGINE_INITIAL_PROCESSOR_HPP

#include "engine/cell_manager.hpp"
#include "engine/light_manager.hpp"
#include "engine/static_manager.hpp"
#include "record/io.hpp"
#include "records.hpp"

namespace engine {

class InitialProcessor {
 private:
  LightManager *lightMgr;
  StaticManager *staticMgr;
  InteriorCellManager *interiorCellMgr;

 public:
  InitialProcessor(LightManager *lightMgr,
                   StaticManager *staticMgr,
                   InteriorCellManager *interiorCellMgr) :
      lightMgr(lightMgr),
      staticMgr(staticMgr),
      interiorCellMgr(interiorCellMgr) {}

  template<class R>
  void readRecord(std::istream &is) {
    record::skipRecord(is);
  }
};

template<>
void InitialProcessor::readRecord<record::STAT>(std::istream &);

template<>
void InitialProcessor::readRecord<record::LIGH>(std::istream &);

template<>
void InitialProcessor::readRecord<record::CELL>(std::istream &);

} // namespace engine

#endif // OPENOBLIVION_ENGINE_INITIAL_PROCESSOR_HPP
