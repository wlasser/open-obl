#ifndef OPENOBLIVION_RESOLVERS_HPP
#define OPENOBLIVION_RESOLVERS_HPP

#include "record/formid.hpp"
#include "record/records_fwd.hpp"
#include "record/reference_records.hpp"
#include "meta.hpp"
#include <absl/container/flat_hash_map.h>
#include <boost/mp11.hpp>
#include <gsl/gsl>
#include <OgreSceneManager.h>
#include <tl/optional.hpp>

namespace oo {

/// Provides the member constant `value` equal to `true` if `R` is a
/// specialization of `record::Record`, and `false` otherwise.
template<class R>
struct is_record : std::false_type {};

template<class T, uint32_t c>
struct is_record<record::Record<T, c>> : std::true_type {};

template<class R> inline constexpr bool is_record_v = is_record<R>::value;

/// Stores all base records of a given type and grants access via their BaseId.
/// \tparam R A record::Record. Specifically, the expression `is_record_v<R>`
///           must evaluate to true.
///
/// At runtime it is necessary to convert BaseIds into the base records that
/// they identify, usually to query information about the record or to create a
/// concrete realization of it represented by a reference record. Call the
/// latter operation **cite** (because it creates a reference, get it? I'm
/// really struggling to think of a good name here). Occasionally it is also
/// necessary to modify base records directly; OBSE provides several functions
/// that do this.
///
/// This class abstracts away the lifetime difference of records obtained from
/// esp files (call these 'esp records'), and those obtained from ess files or
/// generated on the fly (call these 'ess records).
///
/// \remark This class does *not* support deferred loading of records or the
///         loading of records located in hierarchical top groups, namely
///         record::CELL, record::WRLD, and record::DIAL.
template<class R, class Id = BaseId>
class Resolver {
 public:
  using IdType = Id;

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
  absl::flat_hash_map<IdType, RecordEntry> mRecords{};

  using RecordIterator = typename decltype(mRecords)::iterator;

 public:

  /// Insert an esp record or replace an existing one, doing nothing if baseId
  /// refers to an ess record.
  /// \return The iterator component points to the inserted or replaced esp
  ///         record, or already existing ess record. The boolean component is
  ///         true if insertion *or assignment* took place, and false otherwise.
  std::pair<RecordIterator, bool>
  insertOrAssignEspRecord(IdType baseId, const R &rec);

  /// Insert an esp record if there is not esp or ess record with that baseId.
  /// \return The iterator component points to the inserted or already existing
  ///         esp record. The boolean component is true if insertion took place,
  ///         and false otherwise.
  std::pair<RecordIterator, bool>
  insertEspRecord(IdType baseId, const R &rec);

  /// The integer representation of the record type.
  constexpr static inline uint32_t RecordType{R::RecordType};

  /// The underlying raw record type.
  using RawType = typename R::Raw;

  static_assert(is_record_v<R>, "Template parameter must be a Record");

  /// Return the base record, performing disk io if necessary.
  tl::optional<const R &> get(IdType baseId) const;

  /// \overload get(IdType)
  tl::optional<R &> get(IdType baseId);

  /// Checks if there is a record of type R with the baseId.
  bool contains(IdType baseId) const;

  /// Insert a new ess record or replace an existing one.
  /// \return true if insertion took place and false if assignment took place.
  bool insertOrAssign(IdType baseId, const R &rec);

  /// Insert a new ess record, doing nothing if an esp or ess record already
  /// exists with that baseId.
  bool insert(IdType baseId, const R &rec);
};

/// Used for specializing the return type of citeRecord.
/// This should be specialized for each type that citeRecord is specialized for
/// in order to specify the return type of that specialization. Every
/// specialization of CiteRecordTrait should define a public typedef or type
/// alias called `type` for a reference record type, namely
/// record::ACHR, record::ACRE, or any of the record::REFR_xxxx types.
/// \see reference_records.hpp
template<class R>
struct CiteRecordTrait {};

/// Construct a reference record representing a concrete realization of a base
/// record.
// \remark This function template is *not* intended for producing a concrete
///        realization of a record within the scope of the *rendering engine*;
///        see the function template reifyRecord for that
/// \tparam R A record::Record. Specifically, the expression
///           `std::is_same_v<R, record::Record<R::Raw, R::RecordType>` must
///           evaluate to true.
/// \param rec The base record to cite.
/// \param refId The RefId to give the returned reference record. One will be
///              generated if refId is not provided.
///
/// \remark This function template should be specialized for each record::Record
///         where it makes sense.
/// \remark By a 'reference record' it is meant a record::ACHR, record::ACRE, or
///         any of the record::REFR_xxxx types. \see reference_records.hpp
template<class R>
typename CiteRecordTrait<R>::type
citeRecord(const R &baseRec, tl::optional<RefId> refId);

/// Used for specializing the return type of reifyRecord and the resolvers that
/// must be supplied.
/// This should be specialized for each type that reifyRecord is specialized for
/// in order to specify the return type of that specialization. Every
/// specialization of ReifyRecordTrait should define
/// - a public typedef or type alias called `type` equal to the desired return
///   type
/// - a public typedef or type alias called `resolvers` equal to a tuple of
///   const lvalue references to instantiations of Resolver<>. The tuple shall
///   not have more than one element of the same type.
template<class R>
struct ReifyRecordTrait {};

/// Construct a concrete realization of a reference record within the scope of
/// the rendering engine.
/// \tparam R A reference record.
/// \param scnMgr The reified object is constructed in and owned by the
///               Ogre::SceneManager; it is not the caller's responsibility to
///               known how to link the possible components of the type together
///               in a scene.
template<class R>
typename ReifyRecordTrait<R>::type
reifyRecord(const R &refRec, gsl::not_null<Ogre::SceneManager *> scnMgr,
            typename ReifyRecordTrait<R>::resolvers resolvers);

/// Metafunction mapping `Record` to `oo::Resolver<Record>`.
/// An optional `Id` can be specified to change the backing id from `oo::BaseId`
/// to `oo::RefId`, or other.
template<class Record, class Id = oo::BaseId> struct add_resolver {
  static_assert(is_record_v<Record>);
  using type = oo::Resolver<Record, Id>;
};
template<class Record, class Id = oo::BaseId>
using add_resolver_t = typename add_resolver<Record, Id>::type;

/// Metafunction mapping `Record` to `oo::Resolver<Record, oo::RefId>`.
template<class Record> struct add_refr_resolver {
  static_assert(is_record_v<Record>);
  using type = oo::Resolver<Record, oo::RefId>;
};
template<class Record>
using add_refr_resolver_t = typename add_refr_resolver<Record>::type;

/// Given a tuple of references to resolvers, return a tuple containing a subset
/// of those references.
template<class ... Records, class Tuple>
constexpr auto getResolvers(Tuple &&resolvers) {
  using Types = boost::mp11::mp_transform<std::decay_t,
                                          std::remove_reference_t<Tuple>>;
  return std::tie(std::get<
      boost::mp11::mp_find<Types, add_resolver_t<Records>>::value>(
      std::forward<Tuple>(resolvers))...);
}

/// Given a tuple of references to reference resolvers, return a tuple
/// containing a subset of those references.
template<class ... Records, class Tuple>
constexpr auto getRefrResolvers(Tuple &&resolvers) {
  using Types = boost::mp11::mp_transform<std::decay_t,
                                          std::remove_reference_t<Tuple>>;
  return std::tie(std::get<
      boost::mp11::mp_find<Types, add_resolver_t<Records, oo::RefId>>::value>(
      std::forward<Tuple>(resolvers))...);
}

/// Convenience alias for a collection of reference record resolvers.
template<class ... Records>
using RefrResolverTuple = std::tuple<oo::Resolver<Records, oo::RefId> &...>;

/// Convenience alias for a collection of base record resolvers.
template<class ... Records>
using ResolverTuple = std::tuple<const oo::Resolver<Records> &...>;

/// Convenience wrapper for std::get over a RefrResolverTuple.
template<class Record, class Tuple>
constexpr auto &&getRefrResolver(Tuple &&resolvers) {
  using Types = boost::mp11::mp_transform<std::decay_t,
                                          std::remove_reference_t<Tuple>>;
  return std::get<boost::mp11::mp_find<Types,
                                       add_refr_resolver_t<Record>>::value>(
      std::forward<Tuple>(resolvers));
}

/// Convenience wrapper for std::get over a ResolverTuple.
template<class Record, class Tuple>
constexpr auto &&getResolver(Tuple &&resolvers) {
  using Types = boost::mp11::mp_transform<std::decay_t,
                                          std::remove_reference_t<Tuple>>;
  return std::get<boost::mp11::mp_find<Types,
                                       add_resolver_t<Record>>::value>(
      std::forward<Tuple>(resolvers));
}

/// A tuple of all the base records which have resolvers.
using BaseRecords = std::tuple<record::RACE, record::LTEX, record::ACTI,
                               record::DOOR, record::LIGH, record::STAT,
                               record::GRAS, record::TREE, record::NPC_,
                               record::WTHR, record::CLMT, record::CELL,
                               record::WRLD, record::LAND, record::WATR>;
/// A tuple of all the reference records which have resolvers.
using RefrRecords = std::tuple<record::REFR_ACTI, record::REFR_DOOR,
                               record::REFR_LIGH, record::REFR_STAT,
                               record::REFR_NPC_>;

/// A tuple of all the base resolvers.
using BaseResolvers = boost::mp11::mp_transform<oo::add_resolver_t,
                                                BaseRecords>;
/// A tuple of all the reference resolvers.
using RefrResolvers = boost::mp11::mp_transform<oo::add_refr_resolver_t,
                                                RefrRecords>;

/// A tuple of lvalue references to all the base resolvers.
using BaseResolversRef = boost::mp11::mp_transform<std::add_lvalue_reference_t,
                                                   BaseResolvers>;
/// A tuple of lvalue references to all the reference resolvers.
using RefrResolversRef = boost::mp11::mp_transform<std::add_lvalue_reference_t,
                                                   RefrResolvers>;

//===----------------------------------------------------------------------===//
// Resolver member function implementations
//===----------------------------------------------------------------------===//

template<class R, class IdType>
std::pair<typename Resolver<R, IdType>::RecordIterator, bool>
Resolver<R, IdType>::insertOrAssignEspRecord(IdType baseId, const R &rec) {
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

template<class R, class IdType>
std::pair<typename Resolver<R, IdType>::RecordIterator, bool>
Resolver<R, IdType>::insertEspRecord(IdType baseId, const R &rec) {
  return mRecords.try_emplace(baseId, std::in_place_index<0>, rec, tl::nullopt);
}

template<class R, class IdType>
tl::optional<const R &> Resolver<R, IdType>::get(IdType baseId) const {
  const auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  const std::variant<std::pair<const R, tl::optional<R>>, R> &entry{it->second};
  if (entry.index() == 0) {
    const std::pair<const R, tl::optional<R>> &pair{std::get<0>(entry)};
    return pair.second ? *pair.second : pair.first;
  } else {
    return std::get<1>(entry);
  }
}

template<class R, class IdType>
tl::optional<R &> Resolver<R, IdType>::get(IdType baseId) {
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

template<class R, class IdType>
bool Resolver<R, IdType>::contains(IdType baseId) const {
  return mRecords.find(baseId) != mRecords.end();
}

template<class R, class IdType>
bool Resolver<R, IdType>::insertOrAssign(IdType baseId, const R &rec) {
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

template<class R, class IdType>
bool Resolver<R, IdType>::insert(IdType baseId, const R &rec) {
  return mRecords.try_emplace(baseId, std::in_place_index<1>, rec).second;
}

} // namespace oo

#endif // OPENOBLIVION_RESOLVERS_HPP
