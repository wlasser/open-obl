#ifndef OPENOBLIVION_ATMOSPHERE_HPP
#define OPENOBLIVION_ATMOSPHERE_HPP

#include "resolvers/resolvers.hpp"
#include "time_manager.hpp"
#include <OgreSceneManager.h>
#include <array>
#include <random>
#include <vector>

namespace record::raw {
union Color;
} // namespace record::raw

namespace oo {

/// Represents an instance of a particular weather type.
///
/// The atmospheric properties (mostly colours) of the weather are set from a
/// `record::WTHR` record and given at discrete qualitative times of day:
/// sunrise, daytime, sunset, and nighttime. The main use of this class is to
/// aid in interpolating the atmospheric properties between those times.
///
/// Times are not passed to the methods of this class directly because the
/// sunrise/sunset times of the climate are required to convert qualitative
/// times of day into actual times. Instead, times are represented by a
/// `chrono::QualitativeTimeOfDay` and `float` pair `(tod, t)` where
/// `t ∈ [0, 1]` represents how far *towards* `tod` the time is from the
/// previous qualitative time of day. That is, `(tod, 0.0f)` represents the
/// time of day before `tod`, `(tod, 1.0f)` represents `tod`, and `(tod, t)`
/// for `t ∈ (0.0f, 1.0f)` linearly interpolates between those two times.
///
/// For example, `oo::chrono::Sunrise, 1.0f` represents the *middle* of
/// sunrise---halfway between the climate's sunrise begin and sunrise end
/// times---and `oo::chrono::Daytime, 0.5f` represents halfway between the
/// middle of sunrise and the start of daytime.
class Weather {
 public:
  explicit Weather(const record::WTHR &rec);

  oo::BaseId getBaseId() const noexcept;

  /// Colour getters
  /// @{
  Ogre::ColourValue
  getAmbientColor(chrono::QualitativeTimeOfDay tod, float t = 1.0f) const;
  Ogre::ColourValue
  getSunlightColor(chrono::QualitativeTimeOfDay tod, float t = 1.0f) const;
  Ogre::ColourValue
  getLowerSkyColor(chrono::QualitativeTimeOfDay tod, float t = 1.0f) const;
  Ogre::ColourValue
  getUpperSkyColor(chrono::QualitativeTimeOfDay tod, float t = 1.0f) const;
  Ogre::ColourValue
  getLowerCloudColor(chrono::QualitativeTimeOfDay tod, float t = 1.0f) const;
  Ogre::ColourValue
  getUpperCloudColor(chrono::QualitativeTimeOfDay tod, float t = 1.0f) const;
  Ogre::ColourValue
  getSunColor(chrono::QualitativeTimeOfDay tod, float t = 1.0f) const;
  Ogre::ColourValue
  getHorizonColor(chrono::QualitativeTimeOfDay tod, float t = 1.0f) const;
  /// @}

  /// Set `Ogre::SceneManager'`s sky dome to use this weather's material and
  /// cloud textures. This only needs to be called when the weather changes.
  /// \todo Support interpolation of sky domes between weather transitions.
  void setSkyDome(Ogre::SceneManager *scnMgr);

  /// Set the fog colour and visibility distances based on the time of day.
  void setFog(Ogre::SceneManager *scnMgr,
              chrono::QualitativeTimeOfDay tod, float t = 1.0f) const;

  /// Set the shader uniforms of the sky dome material based on the time of day.
  void setSkyMaterial(chrono::QualitativeTimeOfDay tod, float t = 1.0f) const;

  /// Weather-related constants
  /// These are needed by both `oo::World` and `oo::Weather`; putting them here
  /// is as good a place as any.
  ///@{

  /// Distance from the camera to the sun billboard, in meters.
  static constexpr float SUN_DISTANCE{6000.0f * 1.414213562f};
  /// Width and height of the sun billboard, in meters.
  static constexpr float SUN_WIDTH{5000.0f};
  /// Half-extent of the cloud sky box, in meters.
  static constexpr float CLOUD_HEIGHT{3500.0f};
  /// Half-extent of the sky dome, in meters.
  /// \remark OGRE's sky dome is actually a cube, with clever UV mapping to look
  ///         like a sphere.
  static constexpr float SKY_HEIGHT{SUN_DISTANCE + SUN_WIDTH / 0.70710678f};

  ///@}

 private:
  oo::BaseId mBaseId;
  Ogre::TexturePtr mLowerCloudsTex;
  Ogre::TexturePtr mUpperCloudsTex;
  Ogre::MaterialPtr mSkyMaterial;
  Ogre::MaterialPtr mCloudsMaterial;
  // TODO: Support rain

  constexpr static const char *SKY_BASE_MATERIAL{"__skyMaterial"};
  constexpr static const char *CLOUD_BASE_MATERIAL{"__cloudMaterial"};

  /// Linearly interpolate from the quantity `a` at `t = 0` to the quantity
  /// `b` at `t = 1`.
  /// \tparam T should be an element of a vector space.
  template<class T> T lerp(float t, const T &a, const T &b) const noexcept {
    return (1.0f - t) * a + t * b;
  }

  /// Interpolate a quantity returned by the `getter`.
  /// \tparam T is anything allowed by `lerp()`.
  /// \tparam Getter is a function object taking a
  ///                `chrono::QualitativeTimeOfDay` and returning an object of
  ///                type `T`.
  template<class T, class Getter>
  T interp(chrono::QualitativeTimeOfDay tod, float t,
           Getter &&getter) const noexcept;

  /// Interpolate a colour returned by the `getter`.
  /// This is a shorthand to avoid wrapping `mColors` when interpolating sky
  /// colours.
  /// \tparam Getter is a function object taking an `oo::Weather::Colors` and
  ///                returning an `Ogre::Colourvalue`.
  template<class Getter> Ogre::ColourValue
  getColor(chrono::QualitativeTimeOfDay tod, float t,
           Getter &&getter) const noexcept;

  /// Convert a `record::raw::Color` to an `Ogre::ColourValue`.
  // TODO: Move this to conversions.hpp
  Ogre::ColourValue makeColor(record::raw::Color c) const noexcept;

  struct Colors {
    Ogre::ColourValue lowerSky;
    Ogre::ColourValue upperSky;
    Ogre::ColourValue lowerClouds;
    Ogre::ColourValue upperClouds;
    Ogre::ColourValue fog;
    Ogre::ColourValue horizon;
    Ogre::ColourValue ambient;
    Ogre::ColourValue sun;
    Ogre::ColourValue sunlight;
    Ogre::ColourValue stars;
  };

  /// Environment colours for sunrise, day, sunset, and night, in that order.
  std::array<Colors, 4u> mColors;

  struct FogDistance {
    float near{};
    float far{};
  };

  /// Near and far fog distances for sunrise, day, sunset, and night, in that
  /// order.
  std::array<FogDistance, 4u> mFogDistances;
};

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

//===----------------------------------------------------------------------===//
// Function template definitions
//===----------------------------------------------------------------------===//

template<class T, class Getter>
T oo::Weather::interp(chrono::QualitativeTimeOfDay tod, float t,
                      Getter &&getter) const noexcept {
  if (tod == chrono::Sunrise) {
    // Blending into middle of sunrise from nighttime
    return lerp(t, getter(chrono::Nighttime), getter(chrono::Sunrise));
  } else if (tod == chrono::Daytime) {
    // Blending into daytime from middle of sunrise
    return lerp(t, getter(chrono::Sunrise), getter(chrono::Daytime));
  } else if (tod == chrono::Sunset) {
    // Blending into middle of sunset from daytime
    return lerp(t, getter(chrono::Daytime), getter(chrono::Sunset));
  } else {
    // Blending into nighttime form middle of sunset
    return lerp(t, getter(chrono::Sunset), getter(chrono::Nighttime));
  }
}

template<class Getter> Ogre::ColourValue
oo::Weather::getColor(chrono::QualitativeTimeOfDay tod, float t,
                      Getter &&getter) const noexcept {
  return interp<Ogre::ColourValue>(tod, t, [&](auto x) {
    return getter(mColors[unsigned(x)]);
  });
}

} // namespace oo

#endif // OPENOBLIVION_ATMOSPHERE_HPP
