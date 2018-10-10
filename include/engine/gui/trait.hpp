#ifndef OPENOBLIVION_ENGINE_GUI_TRAIT_HPP
#define OPENOBLIVION_ENGINE_GUI_TRAIT_HPP

#include <boost/algorithm/string/predicate.hpp>
#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace engine::gui {

// Implementation traits have well-defined types linked to their name; <x> is
// always an int, for instance. User traits have different types depending on
// the ui element, but these are still well-defined. This enum is used to
// express the type of the user trait in the interface without using templates
// directly. Unimplemented is used to denote that a particular user trait,
// say <user5>, has no effect.
enum class TraitTypeId : int {
  Unimplemented = 0, Int, Float, Bool, String
};

// Convert a trait type into a trait type id.
template<class T>
constexpr TraitTypeId getTraitTypeId() {
  return TraitTypeId::Unimplemented;
}

template<>
constexpr inline TraitTypeId getTraitTypeId<int>() {
  return TraitTypeId::Int;
}

template<>
constexpr inline TraitTypeId getTraitTypeId<float>() {
  return TraitTypeId::Float;
}

template<>
constexpr inline TraitTypeId getTraitTypeId<bool>() {
  return TraitTypeId::Bool;
}

template<>
constexpr inline TraitTypeId getTraitTypeId<std::string>() {
  return TraitTypeId::String;
}

// If name is the name of a user trait, then return the index of that trait,
// e.g. user12 returns 12.
std::optional<int> getUserTraitIndex(const std::string &name);

// This class simplifies expressing the user trait interface of a ui element,
// instead of writing 4 set_user functions containing disjoint switch
// statements. Passing it a std::tuple of pointers to member variables
// corresponding to the user traits (in order) gives the user automatically
// generated set_user and userTraitType functions.
// A problem is that this does not inherit from UiElement, and so does not
// provide overrides for set_user. These must therefore be implemented (with
// identical implementation) in every class that uses a UserTraitInterface.
template<class ...Ts>
class UserTraitInterface {
 private:
  std::tuple<Ts *...> mPtrs;

  template<std::size_t I>
  using IthType = std::remove_pointer_t
      <std::tuple_element_t<I, decltype(mPtrs)>>;

  template<class T, std::size_t I>
  constexpr void assign(T value) {
    if constexpr (std::is_same_v<IthType<I>, T>) {
      *std::get<I>(mPtrs) = value;
    }
  }

  template<class T, std::size_t ... Is>
  constexpr void set_user_impl(int index, T value, std::index_sequence<Is...>) {
    ((index == Is ? assign<T, Is>(value) : (void) T{}), ...);
  }

  template<std::size_t I>
  constexpr auto getTypeFromIndex() const {
    return static_cast<std::underlying_type_t<TraitTypeId>>(
        getTraitTypeId<IthType<I>>());
  }

  template<std::size_t ... Is>
  constexpr TraitTypeId userTraitTypeImpl(int index,
                                          std::index_sequence<Is...>) const {
    return TraitTypeId(((index == Is ? getTypeFromIndex<Is>() : 0) + ... + 0));
  }

 public:
  using interface_t = std::tuple<Ts ...>;

  explicit UserTraitInterface(std::tuple<Ts *...> ptrs)
      : mPtrs(std::move(ptrs)) {}

  template<class T>
  constexpr void set_user(int index, T value) {
    set_user_impl(index, value, std::index_sequence_for<Ts...>{});
  }

  constexpr TraitTypeId userTraitType(int index) const {
    return userTraitTypeImpl(index, std::index_sequence_for<Ts...>{});
  }
};

// This avoids writing the boilerplate necessary because UserTraitInterface
// does not derived from UiElement. Obviously this is not an optimal solution.
// TODO: Replace UserTraitInterface with boost::hana reflection.
#define BUILD_USER_TRAIT_INTERFACE(interface) \
TraitTypeId userTraitType(int index) const override { \
  return interface.userTraitType(index); \
} \
void set_user(int index, int value) override { \
  return interface.set_user(index, value); \
} \
void set_user(int index, float value) override { \
  return interface.set_user(index, value); \
} \
void set_user(int index, bool value) override { \
  return interface.set_user(index, value); \
} \
void set_user(int index, std::string value) override { \
  return interface.set_user(index, value); \
}

// TraitFun represents a function used to set/compute the value of the dynamic
// representative of a trait. It needs to keep track of the names of its
// immediate dependencies as edges in the dependency graph cannot be drawn until
// all traits have been constructed.
template<class T>
class TraitFun {
 public:
  using function_type = std::function<T(void)>;
  using result_type = T;
 private:
  function_type mFun{};
  std::vector<std::string> mDependencies{};
 public:
  TraitFun() = default;
  explicit TraitFun(const function_type &f) : mFun(f) {}
  explicit TraitFun(function_type &&f) : mFun(f) {}

  void addDependency(std::string dep) {
    mDependencies.push_back(std::move(dep));
  }

  const std::vector<std::string> &getDependencies() const {
    return mDependencies;
  }

  T operator()() const {
    return mFun();
  }
  explicit operator bool() const noexcept {
    return static_cast<bool>(mFun);
  }
};

// Forward declare UiElement
struct UiElement;

// TraitSetterFun represents a function used to set the value of the concrete
// representative of a trait.
template<class T>
using TraitSetterFun = std::function<void(UiElement *, T)>;

// The Trait class encapsulates a dynamic representative of a trait, and should
// be bound to a concrete representative via an appropriate setter.
template<class T>
class Trait {
 private:
  TraitFun<T> mValue{};
  std::string mName{};
  TraitSetterFun<T> mSetter{};
  UiElement *mConcrete{};

  // Attempt to set the trait fun to return the value pointed to by source, only
  // performing the assignment if U == T. Returns true if the assignment is
  // successful and false otherwise.
  template<class U>
  bool setTraitFunFromSource(const U *source) {
    if constexpr (std::is_same_v<U, T>) {
      mValue = TraitFun<T>{[source]() -> T {
        return *source;
      }};
      return true;
    }
    return false;
  }

  template<class Tuple, std::size_t ... Is>
  bool setSourceImpl(int index,
                     const Tuple &tuple,
                     std::index_sequence<Is...>) {
    return ((index == Is ? setTraitFunFromSource(&std::get<Is>(tuple)) : false)
        || ... || false);
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

  Trait(Trait &&) noexcept(std::is_nothrow_move_constructible_v<TraitFun<T>>
      && std::is_nothrow_move_constructible_v<TraitSetterFun<T>>) = default;

  Trait<T> &operator=(Trait &&) noexcept(
  std::is_nothrow_move_assignable_v<TraitFun<T>>
      && std::is_nothrow_move_assignable_v<TraitSetterFun<T>>) = default;

  // Bind this Trait as the concrete representative of a trait in the
  // concreteElement, whose value is modifable using the setter.
  void bind(UiElement *concreteElement, TraitSetterFun<T> setter) {
    mConcrete = concreteElement;
    mSetter = setter;
  }

  // If this trait is a user trait of type T for some slot I, and the given user
  // interface has type T in slot I, then reset this trait's TraitFun to point
  // to the value in slot I of the user interface. If any of this is not true,
  // throw.
  template<class ...Ts>
  void setSource(const std::tuple<Ts...> &userInterface) {
    const int index = [this]() {
      const auto opt{getUserTraitIndex(mName)};
      if (opt) return *opt;
      else throw std::runtime_error("Not a user trait");
    }();
    if (!setSourceImpl(index, userInterface,
                       std::index_sequence_for<Ts...>{})) {
      throw std::runtime_error("Incompatible interface");
    }
  };

  // Calculate the actual value of this trait. This does not update the concrete
  // representative.
  T invoke() const {
    return std::invoke(mValue);
  }

  // Calculate the actual value of this trait and update the concrete
  // representative, if any.
  void update() const {
    if (mConcrete) {
      auto v = invoke();
      std::invoke(mSetter, mConcrete, v);
    }
  }

  const std::vector<std::string> &getDependencies() const {
    return mValue.getDependencies();
  }

  std::string getName() const {
    return mName;
  }
};

} // namespace engine::gui

#endif // OPENOBLIVION_ENGINE_GUI_TRAIT_HPP
