#ifndef OPENOBL_BITFLAG_HPP
#define OPENOBL_BITFLAG_HPP

#include <bitset>
#include <type_traits>

struct BitflagMarker {};

// Alternative to scoped enums for implementing bit flags, without using macros.
// Usage is
//
// struct MyFlags : Bitflag<32, MyFlags> {
//   static constexpr enum_t None{0u};
//   static constexpr enum_t DoFoo{1u};
//   static constexpr enum_t DoBar{1u << 1u};
// };
//
// MyFlags flags{MyFlags::make(MyFlags::DoFoo)};
// if (flags & MyFlags::DoFoo) doFoo();
// if (flags & MyFlags::DoBar) doBar();
//
template<std::size_t N, class Self>
class Bitflag : public BitflagMarker {
 protected:
  using bitset_t = std::bitset<N>;

 private:
  bitset_t mBits{};

  static auto constexpr underlying_type_helper() noexcept {
    if constexpr (N <= 8) return uint8_t{0};
    else if constexpr (8 < N && N <= 16) return uint16_t{0};
    else if constexpr (16 < N && N <= 32) return uint32_t{0};
    else if constexpr (32 < N && N <= 64) return uint64_t{0};
    else {
      static_assert(N > 64, "Bitflag must have 64 or fewer bits");
      return 0u;
    }
  }

  // Needed for workaround on MSVC for underlying_t conversion
#if defined(_MSC_VER)
  template<class T>
  struct id_wrapper { using type = T; };
#endif

 public:
  using underlying_t = decltype(Bitflag::underlying_type_helper());
  static constexpr std::size_t num_bits = N;

 protected:
  class enum_t {
   private:
    friend Bitflag<N, Self>;
    bitset_t mBits{};

   public:
    enum_t() = delete;
    explicit constexpr enum_t(bitset_t bits) noexcept : mBits{bits} {}
    explicit constexpr enum_t(underlying_t val) noexcept : mBits{val} {}
    constexpr operator Self() const noexcept {
      return Self::make(*this);
    }

    constexpr enum_t operator~() const noexcept {
      return enum_t{~mBits};
    }
  };

 public:

  static auto make(underlying_t val) noexcept {
    Self tmp{};
    tmp.mBits = bitset_t{val};
    return tmp;
  }

  static auto make(enum_t bits) noexcept {
    Self tmp{};
    tmp.mBits = bitset_t{bits.mBits};
    return tmp;
  }

  explicit operator bool() const noexcept {
    return mBits.any();
  }

#if defined(_MSC_VER)
  // MSVC complains about 'operator decltype' unless underlying_t is wrapped in
  // a type to delay template instantiation.
  explicit operator typename id_wrapper<underlying_t>::type() const noexcept {
    return static_cast<underlying_t>(mBits.to_ullong());
  }
#else
  explicit operator underlying_t() const noexcept {
      return static_cast<underlying_t>(mBits.to_ullong());
    }
#endif

  friend constexpr Self operator&(const Self &lhs, const Self &rhs) noexcept {
    return enum_t{lhs.mBits & rhs.mBits};
  }

  friend constexpr Self operator|(const Self &lhs, const Self &rhs) noexcept {
    return enum_t{lhs.mBits | rhs.mBits};
  }

  friend constexpr Self operator^(const Self &lhs, const Self &rhs) noexcept {
    return enum_t{lhs.mBits ^ rhs.mBits};
  }

  constexpr Self operator~() const noexcept {
    return enum_t{~mBits};
  }

  friend constexpr bool operator==(const Self &lhs, const Self &rhs) noexcept {
    return lhs.mBits == rhs.mBits;
  }

  friend constexpr bool operator!=(const Self &lhs, const Self &rhs) noexcept {
    return lhs.mBits != rhs.mBits;
  }

  constexpr Self &operator&=(const Self &rhs) noexcept {
    mBits &= rhs.mBits;
    return static_cast<Self &>(*this);
  }

  constexpr Self &operator|=(const Self &rhs) noexcept {
    mBits |= rhs.mBits;
    return static_cast<Self &>(*this);
  }

  constexpr Self &operator^=(const Self &rhs) noexcept {
    mBits ^= rhs.mBits;
    return static_cast<Self &>(*this);
  }
};

#endif // OPENOBL_BITFLAG_HPP
