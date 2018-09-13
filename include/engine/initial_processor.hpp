#ifndef OPENOBLIVION_ENGINE_INITIAL_PROCESSOR_HPP
#define OPENOBLIVION_ENGINE_INITIAL_PROCESSOR_HPP

#include "engine/static_manager.hpp"
#include "engine/cell_manager.hpp"
#include "record/io.hpp"
#include "records.hpp"

namespace engine {

class InitialProcessor {
 private:
  StaticManager *staticMgr;
  InteriorCellManager *interiorCellMgr;

 public:
  InitialProcessor(StaticManager *staticMgr,
                   InteriorCellManager *interiorCellMgr) :
      staticMgr(staticMgr), interiorCellMgr(interiorCellMgr) {}

  template<class R>
  void readRecord(std::istream &is) {
    record::skipRecord(is);
  }
};

template<>
void InitialProcessor::readRecord<record::STAT>(std::istream &);

template<>
void InitialProcessor::readRecord<record::CELL>(std::istream &);

} // namespace engine

#endif // OPENOBLIVION_ENGINE_INITIAL_PROCESSOR_HPP
