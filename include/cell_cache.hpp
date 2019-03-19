#ifndef OPENOBLIVION_CELL_CACHE_HPP
#define OPENOBLIVION_CELL_CACHE_HPP

#include "record/formid.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/fiber/mutex.hpp>
#include <OgreQuaternion.h>
#include <OgreVector3.h>
#include <memory>

namespace oo {

class Cell;
class InteriorCell;
class ExteriorCell;
class World;

class CellCache {
 public:
  using InteriorPtr = std::shared_ptr<InteriorCell>;
  using ExteriorPtr = std::shared_ptr<ExteriorCell>;
  using CellPtr = std::shared_ptr<Cell>;
  using InteriorBuffer = boost::circular_buffer<InteriorPtr>;
  using ExteriorBuffer = boost::circular_buffer<ExteriorPtr>;

  struct GetResult {
    CellPtr cell;
    bool isInterior;
    explicit GetResult(CellPtr cell, bool isInterior)
        : cell(std::move(cell)), isInterior(isInterior) {}
  };

 private:
  InteriorBuffer mInteriors{};
  ExteriorBuffer mExteriors{};
  mutable boost::fibers::mutex mMutex{};

 public:
  explicit CellCache(std::size_t interiorCapacity, std::size_t exteriorCapacity)
      : mInteriors(interiorCapacity), mExteriors(exteriorCapacity) {}

  void push_back(const InteriorPtr &interiorCell);
  void push_back(const ExteriorPtr &exteriorCell);

  /// Move the given cell to the back of its buffer, if it exists.
  void promote(oo::BaseId id);

  const InteriorBuffer &interiors() const;
  const ExteriorBuffer &exteriors() const;

  GetResult get(oo::BaseId id) const;
};

/// Cell to load and where to place the player in it.
/// \todo This is temporary, and needs to be moved/replaced with a better system
struct CellRequest {
  oo::BaseId mCellId{};
  Ogre::Vector3 mPlayerPosition{};
  Ogre::Quaternion mPlayerOrientation{};

  explicit CellRequest(oo::BaseId cellId,
                       const Ogre::Vector3 &playerPosition = Ogre::Vector3::ZERO,
                       const Ogre::Quaternion &playerOrientation = Ogre::Quaternion::IDENTITY)
      : mCellId(cellId),
        mPlayerPosition(playerPosition),
        mPlayerOrientation(playerOrientation) {}
};

struct CellPacket {
  std::shared_ptr<World> mWrld{};
  std::shared_ptr<InteriorCell> mInteriorCell{};
  std::vector<std::shared_ptr<ExteriorCell>> mExteriorCells{};

  Ogre::Vector3 mPlayerPosition{};
  Ogre::Quaternion mPlayerOrientation{};

  explicit CellPacket(std::shared_ptr<World> world,
                      std::shared_ptr<InteriorCell> interiorCell,
                      std::vector<std::shared_ptr<ExteriorCell>> exteriorCells,
                      const Ogre::Vector3 &playerPosition = Ogre::Vector3::ZERO,
                      const Ogre::Quaternion &playerOrientation = Ogre::Quaternion::IDENTITY)
      : mWrld(std::move(world)),
        mInteriorCell(std::move(interiorCell)),
        mExteriorCells(std::move(exteriorCells)),
        mPlayerPosition(playerPosition),
        mPlayerOrientation(playerOrientation) {}
};

} // namespace oo

#endif // OPENOBLIVION_CELL_CACHE_HPP
