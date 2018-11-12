#ifndef OPENOBLIVION_RESOLVERS_HPP
#define OPENOBLIVION_RESOLVERS_HPP

#include "formid.hpp"
#include "resolvers/helpers.hpp"
#include "meta.hpp"
#include <gsl/gsl>
#include <OgreSceneManager.h>
#include <tl/optional.hpp>

/// Stores all base records of a given type and grants access via their BaseId.
/// \tparam R A record::Record. Specifically, the expression
///           `std::is_same_v<R, record::Record<R::Raw, R::RecordType>` must
///           evaluate to true.
///
/// At runtime it is necessary to convert BaseIds into the base records that
/// they identify, usually to query information about the record or to create a
/// concrete realization of it represented by a reference record. Occasionally
/// it is also necessary to modify base records directly; OBSE provides several
/// functions that do this.
///
/// This class abstracts away the lifetime difference of records obtained from
/// esp files (call these 'esp records'), and those obtained from ess files or
/// generated on the fly (call these 'ess records). The intent is that only the
/// public-facing interface will need to be specialized for any given
/// record::Record.
///
/// \remark This class does *not* support deferred loading of records or the
///         loading of records located in hierarchical top groups, namely
///         record::CELL, record::WRLD, and record::DIAL.
template<class R>
class Resolver {
 private:
  /// Holds a record with an immutable backup of the original.
  /// Used to provide something like 'opt-out CoW access' to records.
  /// Isomorphic to the sum type `esp + ess + (esp x ess)`, where `esp` is
  /// short for `esp record` and `ess` is short for `ess record` (see main
  /// description for definitions).
  /// \remark The isomorphic `std::variant<const R, R, std::pair<const R, R>>`
  ///         is more space efficient but also more awkward to use.
  using RecordEntry = std::variant<std::pair<const R, tl::optional<R>>, R>;

  /// Record storage.
  std::unordered_map<BaseId, RecordEntry> mRecords{};

  using RecordIterator = typename decltype(mRecords)::iterator;

  /// Insert an esp record or replace an existing one, doing nothing if baseId
  /// refers to an ess record.
  /// \return The iterator component points to the inserted or replaced esp
  ///         record, or already existing ess record. The boolean component is
  ///         true if insertion *or assignment* took place, and false otherwise.
  std::pair<RecordIterator, bool>
  insertOrAssignEspRecord(BaseId baseId, const R &rec);

  /// \overload insertOrAssignEspRecord(BaseId, const R &)
  std::pair<RecordIterator, bool>
  insertOrAssignEspRecord(BaseId baseId, R &&rec);

  friend class InitialRecordVisitor;
 public:
  /// The integer representation of the record type.
  constexpr static inline uint32_t RecordType{R::RecordType};

  /// The underlying raw record type.
  using RawType = typename R::Raw;

  static_assert(std::is_same_v<R, record::Record<RawType, RecordType>>,
                "Template parameter must be a Record");

  /// Return the base record, performing disk io if necessary.
  tl::optional<const R &> get(BaseId baseId) const;

  /// \overload get(BaseId)
  tl::optional<R &> get(BaseId baseId);

  /// Checks if there is a record of type R with the baseId.
  bool contains(BaseId baseId) const;

  /// Insert a new ess record or replace an existing one.
  /// \return true if insertion took place and false if assignment took place.
  bool insertOrAssign(BaseId baseId, const R &rec);
};

template<class R>
struct ReifyRecordTrait {};

/// Construct a concrete realization of a record.
/// This function template is intended to return both a reference record
/// representing a concrete realization of the base record within the scope of
/// the game engine, and some dependent type representing a concrete
/// realization within the scope of the rendering engine.
/// \tparam R A record::Record. Specifically, the expression
///           `std::is_same_v<R, record::Record<R::Raw, R::RecordType>` must
///           evaluate to true.
/// \param rec The base record to reify.
/// \param scnMgr The reified record is constructed in and owned by the
///               Ogre::SceneManager; it is not the caller's responsibility to
///               known how to link the possible components of the type together
///               in a scene.
/// \param refId The RefId to give the returned reference record. One will be
///              generated if refId is not provided.
///
/// \remark This function template should be specialized for each record::Record
///         where it makes sense.
/// \remark By a 'reference record' it is meant a record::REFR, record::ACHR,
///         or record::ACRE.
template<class R>
typename ReifyRecordTrait<R>::type
reifyRecord(const R &rec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            tl::optional<RefId> refId);

template<class R>
std::pair<typename Resolver<R>::RecordIterator, bool>
Resolver<R>::insertOrAssignEspRecord(BaseId baseId, const R &rec) {
  auto[it, inserted]{mRecords.try_emplace(baseId, std::in_place_index<0>,
                                          rec, tl::nullopt)};
  if (inserted) return {it, inserted};
  if (it->second.index() == 0) {
    std::pair<const R, tl::optional<R>> &pair{std::get<0>(it->second)};
    if (!pair.second) {
      it->second.template emplace<0>(rec, tl::nullopt);
      return {it, true};
    }
  }
  return {it, false};
}

template<class R>
std::pair<typename Resolver<R>::RecordIterator, bool>
Resolver<R>::insertOrAssignEspRecord(BaseId baseId, R &&rec) {
  auto[it, inserted]{mRecords.try_emplace(baseId, std::in_place_index<0>,
                                          rec, tl::nullopt)};
  if (inserted) return {it, inserted};
  if (it->second.index() == 0) {
    std::pair<const R, tl::optional<R>> &pair{std::get<0>(it->second)};
    if (!pair.second) {
      it->second.template emplace<0>(rec, tl::nullopt);
      return {it, true};
    }
  }
  return {it, false};
}

template<class R>
tl::optional<const R &> Resolver<R>::get(BaseId baseId) const {
  const auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  return std::visit(overloaded{
      [](const R &e) { return e; },
      [](auto &&pair) { return pair.second ? *pair.second : pair.first; }
  }, it->second);
}

template<class R>
tl::optional<R &> Resolver<R>::get(BaseId baseId) {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  RecordEntry &entry{it->second};

  if (entry.index() == 0) {
    auto &pair{std::get<0>(entry)};
    if (!pair.second) pair.second.emplace(pair.first);
    return *pair.second;
  }
  return std::get<1>(entry);
}

template<class R>
bool Resolver<R>::contains(BaseId baseId) const {
  return mRecords.find(baseId) != mRecords.end();
}

template<class R>
bool Resolver<R>::insertOrAssign(BaseId baseId, const R &rec) {
  auto[it, inserted]{mRecords.try_emplace(baseId, std::in_place_index<1>, rec)};
  if (inserted) return true;
  return std::visit(overloaded{
      [&rec](R &oldRec) {
        oldRec = rec;
        return false;
      },
      [&rec](auto &&pair) {
        const bool doInsert{!pair.second};
        pair.second = rec;
        return doInsert;
      }
  }, it->second);
}

#endif // OPENOBLIVION_RESOLVERS_HPP
