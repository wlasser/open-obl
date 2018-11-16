#ifndef OPENOBLIVION_REFERENCE_RECORDS_HPP
#define OPENOBLIVION_REFERENCE_RECORDS_HPP

#include "record/io.hpp"
#include "record/record.hpp"
#include "record/subrecords.hpp"
#include <optional>
#include <tuple>

/// Oblivion uses a single REFR record to represent the placed versions of many
/// different base records, and does not provide any kind of marker to denote
/// which base record is being represented. This gives a lot of runtime freedom
/// for creating references, but arguably gives too much; there are particular
/// combinations of subrecords which are theoretically permitted but make no
/// sense, such as XLOC (lockInfo) and XSED (speedTree). Moreover, even when we
/// know which specific type of base record the REFR represents, we are forced to
/// be general and accept everything.

namespace record {

namespace raw {

struct REFRBase {
  std::optional<record::EDID> editorId{};
  record::NAME baseId{};
  std::optional<record::XESP> parent{};

  virtual uint32_t size() const {
    return baseId.entireSize()
        + (editorId ? editorId->entireSize() : 0u)
        + (parent ? parent->entireSize() : 0u);
  }

  virtual std::ostream &write(std::ostream &os) const {
    writeRecord(os, editorId);
    writeRecord(os, baseId);
    writeRecord(os, parent);
    return os;
  }

  virtual std::istream &read(std::istream &is) {
    readRecord(is, editorId);
    readRecord(is, baseId);
    readRecord(is, parent);
    return is;
  }

  virtual ~REFRBase() = default;
};

struct REFRTransformation {
  std::optional<record::XSCL> scale{};
  record::DATA_REFR positionRotation{};

  virtual uint32_t size() const {
    return (scale ? scale->entireSize() : 0u)
        + positionRotation.entireSize();
  }

  virtual std::ostream &write(std::ostream &os) const {
    writeRecord(os, scale);
    writeRecord(os, positionRotation);
    return os;
  }

  virtual std::istream &read(std::istream &is) {
    readRecord(is, scale);
    readRecord(is, positionRotation);
    return is;
  }

  virtual ~REFRTransformation() = default;
};

template<class ...T>
struct RecordTuplifiable {
  using tuple_t = std::tuple<T *...>;
  using const_tuple_t = std::tuple<T const *...>;
  virtual tuple_t asTuple() = 0;
  virtual const_tuple_t asTuple() const = 0;
  virtual ~RecordTuplifiable() = default;
};

#define RECORD_MAKE_AS_TUPLE(...) \
  tuple_t asTuple() override { return {__VA_ARGS__}; } \
  const_tuple_t asTuple() const override { return {__VA_ARGS__}; }

struct REFRTargetable : RecordTuplifiable<std::optional<record::XTRG>> {
  std::optional<record::XTRG> target{};
  RECORD_MAKE_AS_TUPLE(&target);
};

struct REFRMarker : RecordTuplifiable<std::optional<record::XMRK>,
                                      std::optional<record::FNAM_REFR>,
                                      std::optional<record::FULL>,
                                      std::optional<record::TNAM>> {
  std::optional<record::XMRK> mapMarker{};
  std::optional<record::FNAM_REFR> mapFlags{};
  std::optional<record::FULL> markerName{};
  std::optional<record::TNAM> markerType{};
  RECORD_MAKE_AS_TUPLE(&mapMarker, &mapFlags, &markerName, &markerType);
};

struct REFROwnable : RecordTuplifiable<std::optional<record::XOWN>,
                                       std::optional<record::XGLB>,
                                       std::optional<record::XRNK>> {
  std::optional<record::XOWN> owner{};
  std::optional<record::XGLB> ownershipGlobal{};
  std::optional<record::XRNK> ownershipRank{};
  RECORD_MAKE_AS_TUPLE(&owner, &ownershipGlobal, &ownershipRank);
};

struct REFRDoor : RecordTuplifiable<std::optional<record::XTEL>,
                                    std::optional<record::XRTM>,
                                    std::optional<record::XACT>,
                                    std::optional<record::ONAM>> {
  std::optional<record::XTEL> teleport{};
  std::optional<record::XRTM> teleportParent{};
  std::optional<record::XACT> action{};
  std::optional<record::ONAM> openByDefault{};
  RECORD_MAKE_AS_TUPLE(&teleport, &teleportParent, &action, &openByDefault);
};

struct REFRLockable : RecordTuplifiable<std::optional<record::XLOC>> {
  std::optional<record::XLOC> lockInfo{};
  RECORD_MAKE_AS_TUPLE(&lockInfo);
};

struct REFRTree : RecordTuplifiable<std::optional<record::XSED>,
                                    std::optional<record::XLOD>> {
  std::optional<record::XSED> speedTree{};
  std::optional<record::XLOD> lod{};
  RECORD_MAKE_AS_TUPLE(&speedTree, &lod);
};

struct REFRItem : RecordTuplifiable<std::optional<record::XCNT>> {
  std::optional<record::XCNT> count{};
  RECORD_MAKE_AS_TUPLE(&count);
};

struct REFRSoulGem : RecordTuplifiable<std::optional<record::XSOL>> {
  std::optional<record::XSOL> soul{};
  RECORD_MAKE_AS_TUPLE(&soul);
};

struct REFRLeveled : RecordTuplifiable<std::optional<record::XLCM>> {
  std::optional<record::XLCM> levelModifier{};
  RECORD_MAKE_AS_TUPLE(&levelModifier);
};

struct REFRUnused : RecordTuplifiable<std::optional<record::XPCI>,
                                      std::optional<record::FULL>> {
  std::optional<record::XPCI> unusedCellId{};
  std::optional<record::FULL> unusedCellName{};
  RECORD_MAKE_AS_TUPLE(&unusedCellId, &unusedCellName);
};

struct REFRRagdoll : RecordTuplifiable<std::optional<record::XRGD>> {
  std::optional<record::XRGD> ragdollData{};
  RECORD_MAKE_AS_TUPLE(&ragdollData);
};

// TODO: Finish the rest of these

template<class T>
constexpr uint32_t recTypeOfComponent(T *x) {
  return T::value_type::RecordType;
}

template<uint32_t Base, class ...Components>
struct REFR : REFRBase, Components ..., REFRTransformation {
  constexpr static inline uint32_t RecordType = Base;

  uint32_t size() const override {
    // @formatter:off
    return REFRBase::size()
        + (std::apply([](const auto *...x) {
          return ((*x ? (*x)->entireSize() : 0u) + ... + 0);
        }, Components::asTuple()) + ... + 0)
        + REFRTransformation::size();
    // @formatter:on
  }

  std::ostream &write(std::ostream &os) const override {
    REFRBase::write(os);
    (std::apply([&os](const auto *...x) { (writeRecord(os, *x), ...); },
                Components::asTuple()), ...);
    REFRTransformation::write(os);
    return os;
  }

  std::istream &read(std::istream &is) override {
    REFRBase::read(is);
    // Two components can be interleaved, e.g. XSCL sometimes occurs before
    // XRTM, and sometimes occurs after. We cannot therefore treat Components
    // as reorderable blocks.
    auto members{std::tuple_cat(Components::asTuple()...)};
    for (;;) {
      const uint32_t recType{peekRecordType(is)};
      // @formatter:off
      if (!std::apply([&is, recType](auto *...x) {
          return (... || (recTypeOfComponent(x) == recType
            ? (readRecord(is, *x), true) : false));
        }, members)
      ) {
        break;
      }
      // @formatter:on
    }

    REFRTransformation::read(is);
    return is;
  }
};

using REFR_ACTI = raw::REFR<"ACTI"_rec, REFRTargetable>;
using REFR_DOOR = raw::REFR<"DOOR"_rec, REFRDoor, REFRLockable, REFROwnable>;
using REFR_LIGH = raw::REFR<"LIGH"_rec>;
using REFR_MISC = raw::REFR<"MISC"_rec, REFRItem, REFROwnable>;
using REFR_STAT = raw::REFR<"STAT"_rec, REFRTargetable>;

} // namespace raw

template<> std::ostream &
raw::write(std::ostream &os, const raw::REFR_ACTI &t, std::size_t);

template<> std::istream &
raw::read(std::istream &is, raw::REFR_ACTI &t, std::size_t);

template<> std::ostream &
raw::write(std::ostream &os, const raw::REFR_DOOR &t, std::size_t);

template<> std::istream &
raw::read(std::istream &is, raw::REFR_DOOR &t, std::size_t);

template<> std::ostream &
raw::write(std::ostream &os, const raw::REFR_LIGH &t, std::size_t);

template<> std::istream &
raw::read(std::istream &is, raw::REFR_LIGH &t, std::size_t);

template<> std::ostream &
raw::write(std::ostream &os, const raw::REFR_MISC &t, std::size_t);

template<> std::istream &
raw::read(std::istream &is, raw::REFR_MISC &t, std::size_t);

template<> std::ostream &
raw::write(std::ostream &os, const raw::REFR_STAT &t, std::size_t);

template<> std::istream &
raw::read(std::istream &is, raw::REFR_STAT &t, std::size_t);

using REFR_ACTI = Record<raw::REFR_ACTI, "REFR"_rec>;
using REFR_DOOR = Record<raw::REFR_DOOR, "REFR"_rec>;
using REFR_LIGH = Record<raw::REFR_LIGH, "REFR"_rec>;
using REFR_MISC = Record<raw::REFR_MISC, "REFR"_rec>;
using REFR_STAT = Record<raw::REFR_STAT, "REFR"_rec>;

/// This is a placeholder.
/// TODO: Explain this.
using REFR = Record<std::tuple<>, "REFR"_rec>;

BaseId peekBaseOfReference(std::istream &is);

} // namespace record

#endif // OPENOBLIVION_REFERENCE_RECORDS_HPP
