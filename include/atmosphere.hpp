#ifndef OPENOBLIVION_ATMOSPHERE_HPP
#define OPENOBLIVION_ATMOSPHERE_HPP

#include "resolvers/resolvers.hpp"
#include "resolvers/wthr_resolver.hpp"
#include "time_manager.hpp"
#include <OgreSceneManager.h>
#include <random>
#include <vector>

namespace oo {

class Atmosphere {
 private:
  using Resolvers = std::tuple<const Resolver<record::WTHR> &,
                               const Resolver<record::CLMT> &,
                               const Resolver<record::WRLD> &>;
  using WeatherDistribution = std::discrete_distribution<std::size_t>;

  oo::chrono::minutes mSunriseBegin{};
  oo::chrono::minutes mSunriseEnd{};
  oo::chrono::minutes mSunsetBegin{};
  oo::chrono::minutes mSunsetEnd{};
  bool mHasMasser{false};
  bool mHasSecunda{false};
  bool mHasSun{false};
  unsigned int mPhaseLength{0};
  std::vector<oo::Weather> mWeathers{};
  WeatherDistribution mWeatherDistribution{};
  float mVolatility{};
  std::size_t mCurrentWeather{0};

  gsl::not_null<Ogre::SceneManager *> mScnMgr;

  void makeClimateSettings(const record::CLMT &rec);
  void makeWeatherList(const record::CLMT &rec, Resolvers resolvers);
  void makeSun(const record::CLMT &rec);

  constexpr static const char *SUN_NODE{"__sunNode"};
  constexpr static const char *SUN_LIGHT{"__sunLight"};
  constexpr static const char *SUN_BILLBOARD_SET{"__sunBillboardSet"};
  constexpr static const char *SUN_BASE_MATERIAL{"__sunMaterial"};

  /// Return the sun scene node, possibly creating it, along with a boolean
  /// indicating whether it was created or not.
  std::pair<Ogre::SceneNode *, bool> createOrRetrieveSunNode();
  /// Return the sun light, possibly creating it, along with a boolean
  /// indicating whether it was created or not.
  std::pair<Ogre::Light *, bool> createOrRetrieveSunLight();
  /// Return the sun billboard set, possibly creating it and the sun billboard,
  /// along with a boolean indicating whether it was created or not.
  std::pair<Ogre::BillboardSet *, bool> createOrRetrieveSunBillboardSet();
  /// Return the sun material for the given climate, possibly creating it,
  /// along with a boolean indicating whether it was created or not.
  std::pair<Ogre::MaterialPtr, bool>
  createOrRetrieveSunMaterial(const record::CLMT &rec);

  /// Return the sun scene node, or nullptr if it doesn't exist
  Ogre::SceneNode *getSunNode() const noexcept;
  /// Return the sun light, or nullptr if it doesn't exist
  Ogre::Light *getSunLight() const noexcept;
  /// Return the sun billboard, or nullptr if it doesn't exist
  Ogre::Billboard *getSunBillboard() const noexcept;

  /// Get the position of the sun at the given time of day, relative to an
  /// observer.
  Ogre::Vector3 getSunPosition(const chrono::minutes &time) const;

  /// 'Simple' implementation of `getSunPosition()`.
  /// This function assumes that the sun moves at a uniform velocity in a
  /// semicircular arc through the zenith, rising above the east horizon at the
  /// beginning of sunrise and setting below the west horizon at the end of
  /// sunset.
  Ogre::Vector3 getSunPositionSimple(const chrono::minutes &time) const;

  /// 'Physical' implementation of `getSunPosition()`.
  /// The position of the sun on the celestial sphere is calculated based on the
  /// time of year. This position is converted into an apparent position as
  /// seen by an observer at a fixed longitude and latitude at the given time
  /// of day.
  ///
  /// Obviously this assumes that the solar system in-game behaves in a similar
  /// manner to the real-world. Since a lot of the necessary astronomical
  /// information is---justifiably, since why would anybody care---missing from
  /// the game, we have to take some liberties and assume that Nirn is like
  /// Earth in a lot of ways. In particular, we assume the same obliquity of
  /// the ecliptic and roughly the same orbital shape. These aren't necessary
  /// assumptions, one could take whatever values they like, but copying Earth
  /// keeps things looking realistic.
  ///
  /// Because we don't know the size of Nirn, we don't try to update the
  /// observer's (geographic) latitude and longitude based on their position in
  /// the game world. Instead, we just put them in the northern hemisphere along
  /// the prime meridian.
  ///
  /// \todo Use the actual time of year, instead of the epoch.
  Ogre::Vector3 getSunPositionPhysical(const chrono::minutes &time) const;

  struct EquatorialCoordinates {
    Ogre::Radian rightAscension{};
    Ogre::Radian declination{};
  };

  /// Return the position of the sun in equatorial coordinates based on the time
  /// of year. This is an implementation function for `getSunPositionPhysical`.
  EquatorialCoordinates getSunEquatorialCoordinates() const;

  /// Set the sunrise and sunset times based on the given sun coordinates and
  /// observer latitude.
  void setSunriseSunsetTimes(const Ogre::Radian &declination,
                             const Ogre::Radian &latitude);

  /// Split a time in minutes from 12:00 am into a `(time of day, t)` pair
  /// required by `oo::Weather`.
  std::pair<chrono::QualitativeTimeOfDay, float>
  splitTime(const chrono::minutes &time) const noexcept;

 public:
  Atmosphere(oo::BaseId wrldId, gsl::not_null<Ogre::SceneManager *> scnMgr,
             Resolvers resolvers);
  ~Atmosphere();

  Atmosphere(const Atmosphere &) = delete;
  Atmosphere &operator=(const Atmosphere &) = delete;
  Atmosphere(Atmosphere &&) = delete;
  Atmosphere &operator=(Atmosphere &&) = delete;

  void update(const oo::chrono::minutes &time);
};

} // namespace oo

#endif // OPENOBLIVION_ATMOSPHERE_HPP
