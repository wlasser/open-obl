#ifndef OPENOBL_PERSISTENT_REFERENCE_LOCATOR_HPP
#define OPENOBL_PERSISTENT_REFERENCE_LOCATOR_HPP

#include "record/formid.hpp"
#include "resolvers/wrld_resolver.hpp"
#include <boost/fiber/mutex.hpp>
#include <tl/optional.hpp>

namespace oo {

/// Provides lookup of the location of persistent reference records.
/// The individual reference resolvers can be used to find a given reference
/// record by its `RefId`, but this does not provide any information as to
/// where the reference record is in the game world. This class keeps track of
/// the locations of persistent reference records. For records that are in
/// interior cells the `BaseId` of the cell is given. For record that are in
/// exterior cells the `BaseId` of the worldspace and the `CellIndex` of the
/// cell are given. At a larger cost and with a `record::WRLD` resolver, the
/// `BaseId` of the exterior cell can also be queried.
class PersistentReferenceLocator {
 private:
  /// Represents an interior or exterior location. Both are packed in one struct
  /// to avoid the size overhead of a `std::variant`.
  struct Location {
    oo::BaseId mWrldId{};
    oo::BaseId mCellId{};
    oo::CellIndex mCellIndex{};
    /// Create an exterior location
    explicit Location(oo::BaseId wrldId, oo::CellIndex cellIndex)
        : mWrldId(wrldId), mCellIndex(cellIndex) {}
    /// Create an interior location
    explicit Location(oo::BaseId cellId) : mCellId(cellId) {}
  };

  std::unordered_map<oo::RefId, Location> mLocations{};
  mutable boost::fibers::mutex mMutex{};

 public:
  /// Get the `CellIndex` of the exterior cell that the reference record is in,
  /// or an empty optional if the reference record does not exist or is in an
  /// interior cell.
  tl::optional<oo::CellIndex> getCellIndex(oo::RefId refId) const noexcept;

  /// Get the `BaseId` of the worldspace that the reference record is in, or
  /// an empty optional if the reference record does not exists or is in an
  /// interior cell.
  tl::optional<oo::BaseId> getWorldspace(oo::RefId refId) const noexcept;

  /// Get the `BaseId` of the interior cell that the reference record is in,
  /// or an empty optional if the reference record does not exist or is in an
  /// exterior cell.
  tl::optional<oo::BaseId> getCell(oo::RefId refId) const noexcept;

  /// Find all reference records which belong to the given interior cell.
  std::unordered_set<oo::RefId>
  getRecordsInCell(oo::BaseId cellId) const noexcept;

  /// Find all reference records which belong to the exterior cell with the
  /// given coordinates, in the given worldspace.
  std::unordered_set<oo::RefId>
  getRecordsInCell(oo::BaseId wrldId, oo::CellIndex cellIndex) const noexcept;

  /// Record the position of a persistent reference record that is in an
  /// interior cell. Overwrites any existing record with that `refId`, if any.
  void insert(oo::RefId refId, oo::BaseId cellId) noexcept;

  /// Record the position of a persistent reference record that is in an
  /// exterior cell. Overwrites any existing record with that `refId`, if any.
  void insert(oo::RefId refId,
              oo::BaseId wrldId,
              oo::CellIndex cellIndex) noexcept;
};

} // namespace oo

#endif // OPENOBL_PERSISTENT_REFERENCE_LOCATOR_HPP
