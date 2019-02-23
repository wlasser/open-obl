#ifndef OPENOBLIVION_TIME_MANAGER_HPP
#define OPENOBLIVION_TIME_MANAGER_HPP

#include <chrono>
#include "game_settings.hpp"

namespace oo {

class TimeManager {
 private:
  TimeManager() = default;

 public:
  static TimeManager &getSingleton();
};

/// \defgroup OpenOblivionChrono
/// Provides in-universe date and time functionality similar to `std::chrono`.
/// The `<chrono>` header provides a nice interface for dealing with time as
/// measured in the 'real world', but beyond a few predefined constants the only
/// tether to reality is the choice of *clock* used to measure time. By changing
/// the clock to something we control completely, we can use the same API to
/// represent time points and durations in the game world itself. In particular,
/// we can represent game time with the same API, but completely independently
/// of, the real world `system_clock`.

/// \ingroup OpenOblivionChrono
/// Aliases C++17 `<chrono>` functionality and implements some C++20 features.
///
/// This namespace aliases the core functionality of `<chrono>` as of C++17,
/// adding a new clock for measuring game time, and adding some of the C++20
/// features---in particular the calendar-oriented functionality---in a manner
/// that reflects the game world rather than reality. For example, the C++20
/// `chrono::weekday` constants are not named `Monday`, `Tuesday` etc., but
/// `Morndas`, `Tirdas`, and so on. Obviously then this is *not* a
/// standards-compliant implementation of `<chrono>`; several artistic and
/// technical liberties have been taken, and pointed out where appropriate.
namespace chrono {

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::time_point;
using std::chrono::time_point_cast;

using std::chrono::nanoseconds;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::minutes;
using std::chrono::hours;
using days   = chrono::duration<int32_t, std::ratio<86400>>;
using weeks  = chrono::duration<int32_t, std::ratio<604800>>;
using months = chrono::duration<int32_t, std::ratio<2629746>>;
using years  = chrono::duration<int32_t, std::ratio<31556952>>;

class day;
class month;
class year;
class weekday;
class month_day;
class year_month;
class year_month_day;

/// Models the *Cpp17Clock* concept.
/// This acts as a replacement for `std::chrono::system_clock`, representing the
/// time as experienced in the game world.
/// Epoch is 12am Sundas 26th of Last Seed.
class GameClock {
 public:
  using duration = std::chrono::milliseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<GameClock>;
  static const bool is_steady = false;

  static time_point now() noexcept;

  /// Update the internal tick count by an external amount of seconds `delta`.
  /// The given `delta` is multiplied by the `record::GLOB` `TimeScale` to
  /// obtain the in-world amount of time.
  /// This should be called exactly once every frame that gameplay is occurring
  /// and the state of the world is advancing alongside the direct experience of
  /// the player.
  static void advance(float delta) noexcept/*C++20: [[expects: delta >= 0]]*/;

  /// Advance the internal tick count by the given duration of game time.
  /// If `t_0` is the value of `now()` before this call and `t_1` is the value
  /// of `now()` immediately after, then `t_1 - t_0 = duration.`
  template<class Duration> static void advance(Duration duration) noexcept
  /*C++20: [[expects : duration >= Duration::zero()]]*/ {
    ticks += chrono::duration_cast<GameClock::duration>(duration).count();
  }

  /// Get the calendar date of the epoch.
  static chrono::year_month_day getEpochDate() noexcept;

  /// Set the clock to the given calendar time.
  static void setDate(const chrono::year_month_day &date) noexcept
  /*C++20: [[expects : date > GameClock::getEpochDate()]]*/;

  /// Get the clock time as a calendar date.
  static chrono::year_month_day getDate() noexcept;

  /// Reset the clock back to the epoch.
  static void reset() noexcept { ticks = 0; }

  /// Update all time-related `record::GLOB` records with the current clock
  /// time. This should be called whenever the clock time is changed, such as
  /// through `advance()` or `setDate()`.
  static void updateGlobals();

  /// Update the clock time to reflect the time-related `record::GLOB` records.
  /// This should be called whenever a time-related global is updated without
  /// the clock's knowledge, such as by a script.
  static void updateFromGlobals();

 private:
  /// Each tick is 1 ms in the game world.
  static uint64_t ticks;
};

/// Time point family
/// Time points as measured on the `GameClock`. These are analogous to
/// `std::chrono::sys_time` and related types.
/// @{
template<class Duration>
using game_time = std::chrono::time_point<GameClock, Duration>;
using game_seconds = game_time<std::chrono::seconds>;
using game_days = game_time<chrono::days>;
/// @}

constexpr day operator+(const day &d, const days &ds) noexcept;
constexpr day operator+(const days &ds, const day &d) noexcept;
constexpr day operator-(const day &d, const days &ds) noexcept;
constexpr days operator-(const day &d1, const day &d2) noexcept;

constexpr month operator+(const month &m, const months &ms) noexcept;
constexpr month operator+(const months &ms, const month &m) noexcept;
constexpr month operator-(const month &m, const months &ms) noexcept;
constexpr months operator-(const month &m1, const month &m2) noexcept;

constexpr year operator+(const year &y, const years &ys) noexcept;
constexpr year operator+(const years &ys, const year &y) noexcept;
constexpr year operator-(const year &y, const years &ys) noexcept;
constexpr years operator-(const year &y1, const year &y2) noexcept;

constexpr weekday operator+(const weekday &wd, const days &ds) noexcept;
constexpr weekday operator+(const days &ds, const weekday &wd) noexcept;
constexpr weekday operator-(const weekday &wd, const days &ds) noexcept;
constexpr days operator-(const weekday &wd1, const weekday &wd2) noexcept;

constexpr year_month operator+(const year_month &ym, const months &dm) noexcept;
constexpr year_month operator+(const months &dm, const year_month &ym) noexcept;
constexpr year_month operator-(const year_month &ym, const months &dm) noexcept;
constexpr months operator-(const year_month &x, const year_month &y) noexcept;
constexpr year_month operator+(const year_month &ym, const years &dy) noexcept;
constexpr year_month operator+(const years &dy, const year_month &ym) noexcept;
constexpr year_month operator-(const year_month &ym, const years &dy) noexcept;

constexpr year_month_day
operator+(const chrono::year_month_day &ymd, const chrono::months &dm) noexcept;
constexpr year_month_day
operator+(const chrono::months &dm, const chrono::year_month_day &ymd) noexcept;
constexpr year_month_day
operator+(const chrono::year_month_day &ymd, const chrono::years &dy) noexcept;
constexpr year_month_day
operator+(const chrono::years &dy, const chrono::year_month_day &ymd) noexcept;
constexpr year_month_day
operator-(const chrono::year_month_day &ymd, const chrono::months &dm) noexcept;
constexpr year_month_day
operator-(const chrono::year_month_day &ymd, const chrono::years &dy) noexcept;

//===----------------------------------------------------------------------===//
// Day
//===----------------------------------------------------------------------===//

/// \warning The `ok()` range is `[0, 31)`, not `[1, 31]`.
class day {
 private:
  uint8_t mD;

 public:
  day() = default;
  explicit constexpr day(unsigned d) noexcept : mD(static_cast<uint8_t>(d)) {}
  explicit constexpr operator unsigned() const noexcept { return mD; }
  constexpr bool ok() const noexcept { return mD < 31; }

  constexpr day &operator+=(const days &ds) noexcept {
    return (*this = *this + ds);
  }

  constexpr day &operator-=(const days &ds) noexcept {
    return (*this = *this - ds);
  }

  constexpr day &operator++() noexcept {
    ++mD;
    return *this;
  }

  constexpr day &operator--() noexcept {
    --mD;
    return *this;
  }

  constexpr const day operator++(int) noexcept {
    const auto old{*this};
    operator++();
    return old;
  }

  constexpr const day operator--(int) noexcept {
    const auto old{*this};
    operator--();
    return old;
  }
};

constexpr day operator+(const day &d, const days &ds) noexcept {
  return day{unsigned(d) + ds.count()};
}

constexpr day operator+(const days &ds, const day &d) noexcept {
  return d + ds;
}

constexpr day operator-(const day &d, const days &ds) noexcept {
  return d + -ds;
}

constexpr days operator-(const day &d1, const day &d2) noexcept {
  return days{int(unsigned(d1)) - int(unsigned(d2))};
}

constexpr bool operator==(const day &x, const day &y) noexcept {
  return unsigned(x) == unsigned(y);
}
constexpr bool operator!=(const day &x, const day &y) noexcept {
  return unsigned(x) != unsigned(y);
}
constexpr bool operator<(const day &x, const day &y) noexcept {
  return unsigned(x) < unsigned(y);
}
constexpr bool operator<=(const day &x, const day &y) noexcept {
  return unsigned(x) <= unsigned(y);
}
constexpr bool operator>(const day &x, const day &y) noexcept {
  return unsigned(x) > unsigned(y);
}
constexpr bool operator>=(const day &x, const day &y) noexcept {
  return unsigned(x) >= unsigned(y);
}

//===----------------------------------------------------------------------===//
// Month
//===----------------------------------------------------------------------===//

/// \warning The `ok()` range is `[0, 12)` not [1, 12]`.
class month {
 private:
  uint8_t mM;

 public:
  month() = default;
  explicit constexpr month(unsigned m) noexcept : mM(static_cast<uint8_t>(m)) {}
  explicit constexpr operator unsigned() const noexcept { return mM; }
  constexpr bool ok() const noexcept { return mM < 12; }

  constexpr month &operator+=(const months &ms) noexcept {
    return (*this = *this + ms);
  }

  constexpr month &operator-=(const months &ms) noexcept {
    return (*this = *this - ms);
  }

  constexpr month &operator++() noexcept { return *this += months{1}; }
  constexpr month &operator--() noexcept { return *this -= months{1}; }

  constexpr const month operator++(int) noexcept {
    const month old{*this};
    operator++();
    return old;
  }

  constexpr const month operator--(int) noexcept {
    const month old{*this};
    operator--();
    return old;
  }
};

constexpr month operator+(const month &m, const months &ms) noexcept {
  const auto sum{static_cast<long long>(unsigned(m)) + ms.count()};
  return month{static_cast<unsigned>(sum >= 0 ? (sum % 12)
                                              : (12 - ((-sum) % 12)))};
}

constexpr month operator+(const months &ms, const month &m) noexcept {
  return m + ms;
}

constexpr month operator-(const month &m, const months &ms) noexcept {
  return m + -ms;
}

constexpr months operator-(const month &m1, const month &m2) noexcept {
  const auto v1{unsigned(m1)};
  const auto v2{unsigned(m2)};
  return months{v1 >= v2 ? (v1 - v2) : (v2 - v1)};
}

constexpr bool operator==(const month &x, const month &y) noexcept {
  return unsigned(x) == unsigned(y);
}
constexpr bool operator!=(const month &x, const month &y) noexcept {
  return unsigned(x) != unsigned(y);
}
constexpr bool operator<(const month &x, const month &y) noexcept {
  return unsigned(x) < unsigned(y);
}
constexpr bool operator<=(const month &x, const month &y) noexcept {
  return unsigned(x) <= unsigned(y);
}
constexpr bool operator>(const month &x, const month &y) noexcept {
  return unsigned(x) > unsigned(y);
}
constexpr bool operator>=(const month &x, const month &y) noexcept {
  return unsigned(x) >= unsigned(y);
}

inline constexpr chrono::month MorningStar{0};
inline constexpr chrono::month SunsDawn{1};
inline constexpr chrono::month FirstSeed{2};
inline constexpr chrono::month RainsHand{3};
inline constexpr chrono::month SecondSeed{4};
inline constexpr chrono::month Midyear{5};
inline constexpr chrono::month SunsHeight{6};
inline constexpr chrono::month LastSeed{7};
inline constexpr chrono::month Heartfire{8};
inline constexpr chrono::month Frostfall{9};
inline constexpr chrono::month SunsDusk{10};
inline constexpr chrono::month EveningStar{11};

//===----------------------------------------------------------------------===//
// Year
//===----------------------------------------------------------------------===//

/// \warning There are no leap years.
class year {
 private:
  int16_t mY;

 public:
  static constexpr year min() noexcept { return year{-32767}; }
  static constexpr year max() noexcept { return year{32767}; }

  year() = default;
  explicit constexpr year(int y) noexcept : mY(static_cast<int16_t>(y)) {}
  explicit constexpr operator int() const noexcept { return mY; }
  constexpr bool ok() const noexcept { return -32767 <= mY && mY <= 32767; }

  constexpr year &operator+=(const years &ys) noexcept {
    return (*this = *this + ys);
  }

  constexpr year &operator-=(const years &ys) noexcept {
    return (*this = *this - ys);
  }

  constexpr year &operator++() noexcept {
    ++mY;
    return *this;
  }

  constexpr year &operator--() noexcept {
    --mY;
    return *this;
  }

  constexpr const year operator++(int) noexcept {
    const auto old{*this};
    operator++();
    return old;
  }

  constexpr const year operator--(int) noexcept {
    const auto old{*this};
    operator--();
    return old;
  }

  constexpr year operator+() noexcept { return *this; }
  constexpr year operator-() noexcept { return year{-mY}; }

  constexpr bool is_leap() noexcept { return false; }
};

constexpr year operator+(const year &y, const years &ys) noexcept {
  return year{int(y) + ys.count()};
}

constexpr year operator+(const years &ys, const year &y) noexcept {
  return y + ys;
}

constexpr year operator-(const year &y, const years &ys) noexcept {
  return y + -ys;
}

constexpr years operator-(const year &y1, const year &y2) noexcept {
  return years{int(y1) - int(y2)};
}

constexpr bool operator==(const year &x, const year &y) noexcept {
  return int(x) == int(y);
}
constexpr bool operator!=(const year &x, const year &y) noexcept {
  return int(x) != int(y);
}
constexpr bool operator<(const year &x, const year &y) noexcept {
  return int(x) < int(y);
}
constexpr bool operator<=(const year &x, const year &y) noexcept {
  return int(x) <= int(y);
}
constexpr bool operator>(const year &x, const year &y) noexcept {
  return int(x) > int(y);
}
constexpr bool operator>=(const year &x, const year &y) noexcept {
  return int(x) >= int(y);
}

//===----------------------------------------------------------------------===//
// Weekday
//===----------------------------------------------------------------------===//

/// \warning Construction from `sys_days` and `local_days` is replaced by
///          construction from `game_days`.
/// \warning `operator[]` is unsupported.
class weekday {
 private:
  uint8_t mWd;

 public:
  weekday() = default;
  explicit constexpr weekday(unsigned wd) noexcept
      : mWd(static_cast<uint8_t>(wd)) {}

  // First day of the epoch is Sundas = 0, so no offset is required.
  /*explicit*/ constexpr weekday(const game_days &dp) noexcept
      : mWd(static_cast<uint8_t>((dp.time_since_epoch() % 7).count())) {}

  explicit constexpr operator unsigned() const noexcept { return mWd; }
  constexpr bool ok() const noexcept { return mWd < 7; }

  constexpr weekday &operator+=(const days &d) noexcept {
    return (*this = *this + d);
  }

  constexpr weekday &operator-=(const days &d) noexcept {
    return (*this = *this - d);
  }

  constexpr weekday &operator++() noexcept { return (*this += days{1}); }
  constexpr weekday &operator--() noexcept { return (*this -= days{1}); }

  constexpr const weekday operator++(int) noexcept {
    const auto old{*this};
    operator++();
    return old;
  }

  constexpr const weekday operator--(int) noexcept {
    const auto old{*this};
    operator--();
    return old;
  }
};

constexpr weekday operator+(const weekday &wd, const days &ds) noexcept {
  const auto sum{static_cast<long long>(unsigned(wd)) + ds.count()};
  return weekday{static_cast<unsigned>(sum >= 0 ? (sum % 7)
                                                : (sum - ((-sum) % 7)))};
}

constexpr weekday operator+(const days &ds, const weekday &wd) noexcept {
  return wd + ds;
}

constexpr weekday operator-(const weekday &wd, const days &ds) noexcept {
  return wd + -ds;
}

constexpr days operator-(const weekday &wd1, const weekday &wd2) noexcept {
  const auto v1{unsigned(wd1)};
  const auto v2{unsigned(wd2)};
  return days{v1 >= v2 ? (v1 - v2) : (v2 - v1)};
}

constexpr bool operator==(const weekday &x, const weekday &y) noexcept {
  return unsigned(x) == unsigned(y);
}

constexpr bool operator!=(const weekday &x, const weekday &y) noexcept {
  return unsigned(x) != unsigned(y);
}

inline constexpr chrono::weekday Sundas{0};
inline constexpr chrono::weekday Morndas{1};
inline constexpr chrono::weekday Tirdas{2};
inline constexpr chrono::weekday Middas{3};
inline constexpr chrono::weekday Turdas{4};
inline constexpr chrono::weekday Fredas{5};
inline constexpr chrono::weekday Loredas{6};

//===----------------------------------------------------------------------===//
// Combined Month-Day
//===----------------------------------------------------------------------===//

class month_day {
 private:
  chrono::month mM;
  chrono::day mD;

 public:
  month_day() = default;
  constexpr month_day(const chrono::month &m, const chrono::day &d) noexcept
      : mM(m), mD(d) {}

  constexpr chrono::month month() const noexcept { return mM; }
  constexpr chrono::day day() const noexcept { return mD; }
  /// Whether the month is valid and the day is valid within the month.
  /// \warning The standard requires that February/Sun's Dawn is treated as
  ///          having 29 days here, because we do not know if the year is a leap
  ///          year. Since we do not support leap years, this function assumes
  ///          February/Sun's Dawn has 28 days.
  constexpr bool ok() const noexcept {
    if (!month().ok()) return false;
    if (month() == chrono::MorningStar) return unsigned(day()) < 31;
    else if (month() == chrono::SunsDawn) return unsigned(day()) < 28;
    else if (month() == chrono::FirstSeed) return unsigned(day()) < 31;
    else if (month() == chrono::RainsHand) return unsigned(day()) < 30;
    else if (month() == chrono::SecondSeed) return unsigned(day()) < 31;
    else if (month() == chrono::Midyear) return unsigned(day()) < 30;
    else if (month() == chrono::SunsHeight) return unsigned(day()) < 31;
    else if (month() == chrono::LastSeed) return unsigned(day()) < 31;
    else if (month() == chrono::Heartfire) return unsigned(day()) < 30;
    else if (month() == chrono::Frostfall) return unsigned(day()) < 31;
    else if (month() == chrono::SunsDusk) return unsigned(day()) < 30;
    else if (month() == chrono::EveningStar) return unsigned(day()) < 31;
    else return false;
  }
};

constexpr auto operator/(const chrono::month &m, const chrono::day &d) noexcept
-> chrono::month_day {
  return chrono::month_day(m, d);
}
constexpr auto operator/(const chrono::month &m, int d) noexcept
-> chrono::month_day {
  return chrono::month_day(m, chrono::day(d));
}
constexpr auto operator/(int m, const chrono::day &d) noexcept
-> chrono::month_day {
  return chrono::month_day(chrono::month(m), d);
}
constexpr auto operator/(const chrono::day &d, const chrono::month &m) noexcept
-> chrono::month_day {
  return chrono::month_day(m, d);
}
constexpr auto operator/(const chrono::day &d, int m) noexcept
-> chrono::month_day {
  return chrono::month_day(chrono::month(m), d);
}

constexpr bool operator==(const month_day &x, const month_day &y) {
  return x.month() == y.month() && x.day() == y.day();
}
constexpr bool operator!=(const month_day &x, const month_day &y) {
  return !(x == y);
}
constexpr bool operator<(const month_day &x, const month_day &y) {
  return x.month() < y.month() || (x.month() == y.month() && x.day() < y.day());
}
constexpr bool operator<=(const month_day &x, const month_day &y) {
  return x < y || x == y;
}
constexpr bool operator>(const month_day &x, const month_day &y) {
  return !(x <= y);
}
constexpr bool operator>=(const month_day &x, const month_day &y) {
  return !(x < y);
}

//===----------------------------------------------------------------------===//
// Combined Year-Month
//===----------------------------------------------------------------------===//
class year_month {
 private:
  chrono::year mY;
  chrono::month mM;

 public:
  year_month() = default;
  constexpr year_month(const chrono::year &y, const chrono::month &m) noexcept
      : mY(y), mM(m) {}

  constexpr chrono::year year() const noexcept { return mY; }
  constexpr chrono::month month() const noexcept { return mM; }

  constexpr bool ok() const noexcept { return mM.ok() && mY.ok(); }

  constexpr year_month &operator+=(const months &dm) noexcept {
    return (*this = *this + dm);
  }
  constexpr year_month &operator-=(const months &dm) noexcept {
    return (*this = *this - dm);
  }
  constexpr year_month &operator+=(const years &dy) noexcept {
    return (*this = *this + dy);
  }
  constexpr year_month &operator-=(const years &dy) noexcept {
    return (*this = *this - dy);
  }
};

constexpr auto operator/(const chrono::year &y, const chrono::month &m) noexcept
-> chrono::year_month {
  return chrono::year_month(y, m);
}
constexpr auto operator/(const chrono::year &y, int m) noexcept
-> chrono::year_month {
  return chrono::year_month(y, chrono::month(m));
}

constexpr bool operator==(const year_month &x, const year_month &y) {
  return x.year() == y.year() && x.month() == y.month();
}
constexpr bool operator!=(const year_month &x, const year_month &y) {
  return !(x == y);
}
constexpr bool operator<(const year_month &x, const year_month &y) {
  return x.year() < y.year() || (x.year() == y.year() && x.month() < y.month());
}
constexpr bool operator<=(const year_month &x, const year_month &y) {
  return x < y || x == y;
}
constexpr bool operator>(const year_month &x, const year_month &y) {
  return !(x <= y);
}
constexpr bool operator>=(const year_month &x, const year_month &y) {
  return !(x < y);
}

constexpr year_month
operator+(const year_month &ym, const months &dm) noexcept {
  auto m{static_cast<long long>(unsigned(ym.month())) + dm.count()};
  if (m < 0) {
    auto y{ym.year() + years{static_cast<int>(m / 12) - 1}};
    return year_month(y, month{static_cast<unsigned>(12 - ((-m) % 12))});
  } else {
    auto y{ym.year() + years{static_cast<int>(m / 12)}};
    return year_month(y, month{static_cast<unsigned>(m % 12)});
  }
}

constexpr year_month
operator+(const months &dm, const year_month &ym) noexcept {
  return ym + dm;
}

constexpr year_month
operator-(const year_month &ym, const months &dm) noexcept {
  return ym + -dm;
}

constexpr months
operator-(const year_month &x, const year_month &y) noexcept {
  return x.year() - y.year()
      + months{static_cast<int>(unsigned(x.month()))
                   - static_cast<int>(unsigned(y.month()))};
}

constexpr year_month
operator+(const year_month &ym, const years &dy) noexcept {
  return (ym.year() + dy) / ym.month();
}

constexpr year_month
operator+(const years &dy, const year_month &ym) noexcept {
  return ym + dy;
}

constexpr year_month
operator-(const year_month &ym, const years &dy) noexcept {
  return ym + -dy;
}

//===----------------------------------------------------------------------===//
// Combined Year-Month-Day
//===----------------------------------------------------------------------===//
class year_month_day {
 private:
  chrono::year mY;
  chrono::month mM;
  chrono::day mD;

 public:
  year_month_day() = default;
  constexpr year_month_day(const chrono::year &y, const chrono::month &m,
                           const chrono::day &d) noexcept
      : mY(y), mM(m), mD(d) {}

  // 238 is the number of days of the epoch from the start of the year.
  /*explicit*/ constexpr year_month_day(const game_days &dp) noexcept
      : mY(dp.time_since_epoch().count() / 365 + 433), mM(0), mD(0) {
    auto d{(dp.time_since_epoch().count() + 238) % 365};
    if (d < 31) mM = chrono::MorningStar;
    else if ((d -= 31) < 28) mM = chrono::SunsDawn;
    else if ((d -= 28) < 31) mM = chrono::FirstSeed;
    else if ((d -= 31) < 30) mM = chrono::RainsHand;
    else if ((d -= 30) < 31) mM = chrono::SecondSeed;
    else if ((d -= 31) < 30) mM = chrono::Midyear;
    else if ((d -= 30) < 31) mM = chrono::SunsHeight;
    else if ((d -= 31) < 31) mM = chrono::LastSeed;
    else if ((d -= 30) < 30) mM = chrono::Heartfire;
    else if ((d -= 30) < 31) mM = chrono::Frostfall;
    else if ((d -= 31) < 30) mM = chrono::SunsDusk;
    else {
      d -= 30;
      mM = chrono::EveningStar;
    }
    mD = chrono::day(d);
  }

  constexpr chrono::year year() const noexcept { return mY; }
  constexpr chrono::month month() const noexcept { return mM; }
  constexpr chrono::day day() const noexcept { return mD; }

  constexpr bool ok() const noexcept {
    return mY.ok() && (mM / mD).ok();
  }

  constexpr year_month_day &operator+=(const chrono::years &dy) noexcept {
    return (*this = *this + dy);
  }
  constexpr year_month_day &operator+=(const chrono::months &dm) noexcept {
    return (*this = *this + dm);
  }
  constexpr year_month_day &operator-=(const chrono::years &dy) noexcept {
    return (*this = *this - dy);
  }
  constexpr year_month_day &operator-=(const chrono::months &dm) noexcept {
    return (*this = *this - dm);
  }

  explicit operator chrono::game_days() const noexcept {
    if (ok()) {
      // TODO: This seems pretty inefficient, can we avoid all the comparisons?
      auto yearDiff{int(mY) - 433};
      auto daysIntoTheYear{unsigned(mD)};
      if (mM >= chrono::SunsDawn) daysIntoTheYear += 31;
      if (mM >= chrono::FirstSeed) daysIntoTheYear += 28;
      if (mM >= chrono::RainsHand) daysIntoTheYear += 31;
      if (mM >= chrono::SecondSeed) daysIntoTheYear += 30;
      if (mM >= chrono::Midyear) daysIntoTheYear += 31;
      if (mM >= chrono::SunsHeight) daysIntoTheYear += 30;
      if (mM >= chrono::LastSeed) daysIntoTheYear += 31;
      if (mM >= chrono::Heartfire) daysIntoTheYear += 31;
      if (mM >= chrono::Frostfall) daysIntoTheYear += 30;
      if (mM >= chrono::SunsDusk) daysIntoTheYear += 30;
      if (mM >= chrono::EveningStar) daysIntoTheYear += 31;

      return chrono::game_days()
          + chrono::days(365 * yearDiff + daysIntoTheYear - 238);
    } else if (mY.ok() && mM.ok()) {
      return chrono::game_days(year_month_day(mY, mM, chrono::day(1)))
          + (mD - chrono::day(1));
    } else return chrono::game_days();
  }
};

constexpr auto
operator/(const chrono::year_month &ym,
          const chrono::day &d) noexcept -> chrono::year_month_day {
  return chrono::year_month_day(ym.year(), ym.month(), d);
}
constexpr auto operator/(const chrono::year_month &ym,
                         int d) noexcept -> chrono::year_month_day {
  return chrono::year_month_day(ym.year(), ym.month(), chrono::day(d));
}
constexpr auto
operator/(const chrono::year &y,
          const chrono::month_day &md) noexcept -> chrono::year_month_day {
  return chrono::year_month_day(y, md.month(), md.day());
}
constexpr auto
operator/(int y,
          const chrono::month_day &md) noexcept -> chrono::year_month_day {
  return chrono::year_month_day(chrono::year(y), md.month(), md.day());
}
constexpr auto
operator/(const chrono::month_day &md,
          const chrono::year &y) noexcept -> chrono::year_month_day {
  return chrono::year_month_day(y, md.month(), md.day());
}
constexpr auto operator/(const chrono::month_day &md,
                         int y) noexcept -> chrono::year_month_day {
  return chrono::year_month_day(chrono::year(y), md.month(), md.day());
}

constexpr bool operator==(const year_month_day &x, const year_month_day &y) {
  return x.year() == y.year() && x.month() == y.month() && x.day() == y.day();
}
constexpr bool operator!=(const year_month_day &x, const year_month_day &y) {
  return !(x == y);
}
constexpr bool operator<(const year_month_day &x, const year_month_day &y) {
  return x.year() < y.year() || (x.year() == y.year() && x.month() < y.month())
      || (x.year() == y.year() && x.month() == y.month() && x.day() < y.day());
}
constexpr bool operator<=(const year_month_day &x, const year_month_day &y) {
  return x < y || x == y;
}
constexpr bool operator>(const year_month_day &x, const year_month_day &y) {
  return !(x <= y);
}
constexpr bool operator>=(const year_month_day &x, const year_month_day &y) {
  return !(x < y);
}

constexpr year_month_day operator+(const chrono::year_month_day &ymd,
                                   const chrono::months &dm) noexcept {
  return ((ymd.year() / ymd.month()) + dm) / ymd.day();
}
constexpr year_month_day operator+(const chrono::months &dm,
                                   const chrono::year_month_day &ymd) noexcept {
  return ymd + dm;
}
constexpr year_month_day operator+(const chrono::year_month_day &ymd,
                                   const chrono::years &dy) noexcept {
  return (ymd.year() + dy) / ymd.month() / ymd.day();
}
constexpr year_month_day operator+(const chrono::years &dy,
                                   const chrono::year_month_day &ymd) noexcept {
  return ymd + dy;
}
constexpr year_month_day operator-(const chrono::year_month_day &ymd,
                                   const chrono::months &dm) noexcept {
  return ymd + -dm;
}
constexpr year_month_day operator-(const chrono::year_month_day &ymd,
                                   const chrono::years &dy) noexcept {
  return ymd + -dy;
}

} // namespace chrono

} // namespace oo

#endif // OPENOBLIVION_TIME_MANAGER_HPP
