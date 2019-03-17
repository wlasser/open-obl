#ifndef OPENOBLIVION_GUI_TRAIT_HPP
#define OPENOBLIVION_GUI_TRAIT_HPP

#include "meta.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace gui {

/// Implementation traits have well-defined types linked to their name; `<x>` is
/// always an `int`, for instance. User traits have different types depending on
/// the ui element, but these are still well-defined. This enum is used to
/// express the type of the user trait in the interface without using templates
/// directly. Unimplemented is used to denote that a particular user trait,
/// say `<user5>`, has no effect.
enum class TraitTypeId : int {
  Unimplemented = 0, Float, Bool, String
};

/// \name Trait type to TraitTypeId conversions
///@{

/// Convert a trait type into a trait type id.
template<class T> [[nodiscard]] constexpr
TraitTypeId getTraitTypeId() noexcept {
  return TraitTypeId::Unimplemented;
}

/// \overload getTraitTypeId
template<> [[nodiscard]] constexpr inline
TraitTypeId getTraitTypeId<float>() noexcept {
  return TraitTypeId::Float;
}

/// \overload getTraitTypeId
template<> [[nodiscard]] constexpr inline
TraitTypeId getTraitTypeId<bool>() noexcept {
  return TraitTypeId::Bool;
}

/// \overload getTraitTypeId
template<> [[nodiscard]] constexpr inline
TraitTypeId getTraitTypeId<std::string>() noexcept {
  return TraitTypeId::String;
}

///@}

/// If `name` is the name of a user trait, then return the index of that trait.
/// For example, `getUserTraitIndex("user12") == 12`.
std::optional<int> getUserTraitIndex(std::string_view name);

/// Simplifies the definition of the user trait interface of a ui element.
/// Passing the `UserTraitInterface` a `std::tuple` of pointers to member
/// variables corresponding to the user traits (in order) gives the inheriting
/// class automatically generated `set_user`, `get_user`, and `userTraitType`
/// functions.
///
/// This class is intended as a convenient alternative to the naive definition
/// of a user trait interface via four `set_user` functions with disjoint switch
/// statements. However, because this class does not inherit from `UiElement`,
/// it does not provide overrides for `set_user`. These must be implemented
/// (likely with identical implementation) in every class that uses
/// `UserTraitInterface.` Consider using the `BUILD_USER_TRAIT_INTERFACE` macro
/// to avoid doing this manually.
/// \todo Replace `UserTraitInterface` with `boost::hana` reflection.
template<class ...Ts>
class UserTraitInterface {
 private:
  std::tuple<Ts *...> mPtrs;

  /// Get the type of the `I`th user trait.
  template<std::size_t I>
  using get = std::remove_pointer_t<std::tuple_element_t<I, decltype(mPtrs)>>;

  /// Set the value of the `I`th user trait.
  template<class T, std::size_t I> constexpr void assign(T value) {
    if constexpr (std::is_same_v<get<I>, T>) {
      *std::get<I>(mPtrs) = value;
    }
  }

  /// Get the value of the `I`th user trait and store it in the variant.
  template<class T, std::size_t I>
  constexpr void store(std::variant<Ts...> &v) {
    if constexpr (std::is_same_v<get<I>, T>) {
      v.template emplace<I>(*std::get<I>(mPtrs));
    }
  }

  template<class T, std::size_t ... Is>
  constexpr void set_user_impl(int index, T value, std::index_sequence<Is...>) {
    ((index == Is ? assign<T, Is>(value) : (void) T{}), ...);
  }

  template<class T, std::size_t ... Is>
  constexpr T get_user_impl(int index, std::index_sequence<Is...>) {
    std::variant<Ts...> vars{};
    ((index == Is ? store<T, Is>(vars) : (void) T{}), ...);
    return std::visit([](auto t) -> T {
      if constexpr (std::is_same_v<decltype(t), T>) return t;
      else throw std::runtime_error("get_user_impl type mismatch");
    }, vars);
  }

  /// Get the integer representation of the TraitTypeId of the `I`th user trait.
  template<std::size_t I> constexpr auto getTypeFromIndex() const {
    return static_cast<std::underlying_type_t<TraitTypeId>>(
        getTraitTypeId<get<I>>());
  }

  template<std::size_t ... Is> constexpr TraitTypeId
  userTraitTypeImpl(int index, std::index_sequence<Is...>) const {
    return TraitTypeId(((index == Is ? getTypeFromIndex<Is>() : 0) + ... + 0));
  }

 public:
  using interface_t = std::tuple<Ts ...>;

  constexpr explicit UserTraitInterface(std::tuple<Ts *...> ptrs) noexcept
      : mPtrs(std::move(ptrs)) {}

  /// Set the user trait at the `index` to `value`.
  /// \tparam T Should be *exactly* the type of the user trait at the `index`.
  template<class T> constexpr void set_user(int index, T value) {
    set_user_impl(index, value, std::index_sequence_for<Ts...>{});
  }

  /// Return the user trait at the `index`.
  template<class T> constexpr T get_user(int index) {
    return get_user_impl<T>(index, std::index_sequence_for<Ts...>{});
  }

  /// Return the user trait at the `index`.
  std::variant<float, bool, std::string> get_user(int index) {
    const TraitTypeId id{userTraitType(index)};
    switch (id) {
      case TraitTypeId::Float: return get_user<float>(index);
      case TraitTypeId::Bool: return get_user<bool>(index);
      case TraitTypeId::String: return get_user<std::string>(index);
      default: throw std::runtime_error("User trait index out of bounds");
    }
  }

  /// Get the trait type of the user trait at the `index`.
  constexpr TraitTypeId userTraitType(int index) const {
    return userTraitTypeImpl(index, std::index_sequence_for<Ts...>{});
  }
};

/// This avoids writing the boilerplate necessary because `UserTraitInterface`
/// does not derived from `UiElement`. \see `UserTraitInterface`
#define BUILD_USER_TRAIT_INTERFACE(interface) \
TraitTypeId userTraitType(int index) const override { \
  return interface.userTraitType(index); \
} \
void set_user(int index, gui::UiElement::UserValue value) override { \
  return std::visit([this, index](auto value) { \
    interface.set_user(index, value); }, value); \
} \
gui::UiElement::UserValue get_user(int index) override { \
  return interface.get_user(index); \
} \
void setOutputUserTraitSources(std::vector<TraitVariant *> traits) const override { \
  for (auto *p : traits) { \
    if (!p) continue; \
    std::visit([&](auto &t) { \
      t.setSource(getUserOutputTraitInterface()); \
    }, *p); \
  } \
}

/// Represents a function used to set/compute the value of the dynamic
/// representative of a trait. This needs to keep track of the names of its
/// immediate dependencies, as edges in the dependency graph cannot be drawn
/// until all traits have been constructed.
template<class T>
class TraitFun {
 public:
  using function_type = std::function<T(void)>;
  using result_type = T;

 private:
  function_type mFun{};
  std::vector<std::string> mDependencies{};

 public:
  TraitFun() noexcept = default;
  explicit TraitFun(const function_type &f) : mFun(f) {}
  explicit TraitFun(function_type &&f) noexcept : mFun(std::move(f)) {}

  void addDependency(std::string dep) {
    mDependencies.push_back(std::move(dep));
  }

  const std::vector<std::string> &getDependencies() const {
    return mDependencies;
  }

  /// Call the stored function.
  /// \pre The `TraitFun` should actually contain a function.
  T operator()() const /*C++20: [[requires: *this]]*/ {
    return mFun();
  }

  /// Checks whether the this contains a callable function.
  explicit operator bool() const noexcept {
    return static_cast<bool>(mFun);
  }
};

class UiElement;

/// Represents a function used to set the value of the concrete representative
/// of a trait.
template<class T> using TraitSetterFun = std::function<void(UiElement *, T)>;

/// The dynamic representative of a trait.
/// Each `Trait<T>` should be bound to a concrete representative via an
/// appropriate `TraitSetterFun<T>`.
template<class T>
class Trait {
 private:
  std::string mName{};
  TraitFun<T> mValue{};
  TraitSetterFun<T> mSetter{};
  UiElement *mConcrete{};

  /// Attempt to set the internal `TraitFun` to return the value pointed to by
  /// `source`, only performing the assignment if `std::is_same_v<U, T>` and if
  /// `source` is not null.
  /// \returns `true` iff the assignment is successful or `source` is null.
  /// \todo Can this take `U` by reference and then be `constexpr`?
  template<class U> bool setTrait(const U *source) {
    if (!source) return true;
    if constexpr (std::is_same_v<U, T>) {
      mValue = TraitFun<T>{[source]() -> T { return *source; }};
      return true;
    }
    return false;
  }

  template<class Tuple, std::size_t ... Is> bool
  setSourceImpl(int i, const Tuple &tuple, std::index_sequence<Is...>) {
    return ((i == Is ? setTrait(std::get<Is>(tuple)) : false) || ... || false);
  };

 public:
  explicit Trait(std::string name, T &&t) : mName(std::move(name)),
                                            mValue([t]() { return t; }) {}
  explicit Trait(std::string name, const T &t) : mName(std::move(name)),
                                                 mValue([t]() { return t; }) {}

  explicit Trait(std::string name, TraitFun<T> t) : mName(std::move(name)),
                                                    mValue(t) {}

  Trait(const Trait<T> &) = default;
  Trait<T> &operator=(const Trait<T> &) = default;

  Trait(Trait &&) noexcept = default;
  Trait<T> &operator=(Trait &&) noexcept = default;

  /// Make this `Trait` the concrete representative of a trait in the
  /// `concreteElement`, whose value is modifiable by the `setter`.
  void bind(UiElement *concreteElement, TraitSetterFun<T> setter) {
    mConcrete = concreteElement;
    mSetter = setter;
  }

  /// If this trait is a user trait of type `T` for some slot `I`, and the given
  /// user interface has type `T` in slot `I`, then reset this trait's
  /// `TraitFun` to point to the value in slot `I` of the user interface.
  /// \throws std::runtime_error if any of the specified conditions are false.
  template<class ...Ts>
  void setSource(const std::tuple<Ts *...> &userInterface) {
    const int idx = [this]() {
      const auto opt{getUserTraitIndex(mName)};
      if (opt) return *opt;
      else throw std::runtime_error("Not a user trait");
    }();
    if (!setSourceImpl(idx, userInterface, std::index_sequence_for<Ts...>{})) {
      throw std::runtime_error("Incompatible interface");
    }
  };

  /// Calculate the actual value of this trait. This does not update the
  /// concrete representative, use `update()` for that.
  T invoke() const {
    return std::invoke(mValue);
  }

  /// Calculate the actual value of this trait and update the concrete
  /// representative, if any.
  void update() const {
    if (mConcrete) {
      auto v = invoke();
      std::invoke(mSetter, mConcrete, v);
    }
  }

  [[nodiscard]] const std::vector<std::string> &getDependencies() const {
    return mValue.getDependencies();
  }

  [[nodiscard]] std::string getName() const {
    return mName;
  }
};

} // namespace gui

#endif // OPENOBLIVION_GUI_TRAIT_HPP
