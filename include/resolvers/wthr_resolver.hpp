#ifndef OPENOBLIVION_WTHR_RESOLVER_HPP
#define OPENOBLIVION_WTHR_RESOLVER_HPP

#include "record/formid.hpp"
#include <OgreColourValue.h>
#include <OgreSharedPtr.h>
#include "time_manager.hpp"
#include <array>

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
  Ogre::MaterialPtr getMaterial() const;

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

 private:
  oo::BaseId mBaseId;
  Ogre::TexturePtr mLowerCloudsTex;
  Ogre::TexturePtr mUpperCloudsTex;
  Ogre::MaterialPtr mSkyDomeMaterial;
  // TODO: Support rain

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

#endif // OPENOBLIVION_WTHR_RESOLVER_HPP
