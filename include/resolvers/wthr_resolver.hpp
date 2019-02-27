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

class Weather {
 public:
  explicit Weather(const record::WTHR &rec);

  oo::BaseId getBaseId() const noexcept;
  Ogre::MaterialPtr getMaterial() const;

  Ogre::ColourValue getAmbientColor(chrono::QualitativeTimeOfDay tod) const;
  Ogre::ColourValue getSunlightColor(chrono::QualitativeTimeOfDay tod) const;
  Ogre::ColourValue getLowerSkyColor(chrono::QualitativeTimeOfDay tod) const;
  Ogre::ColourValue getUpperSkyColor(chrono::QualitativeTimeOfDay tod) const;
  Ogre::ColourValue getLowerCloudColor(chrono::QualitativeTimeOfDay tod) const;
  Ogre::ColourValue getUpperCloudColor(chrono::QualitativeTimeOfDay tod) const;
  Ogre::ColourValue getHorizonColor(chrono::QualitativeTimeOfDay tod) const;

  void setSkyDome(Ogre::SceneManager *scnMgr);
  void setFog(Ogre::SceneManager *scnMgr,
              chrono::QualitativeTimeOfDay tod) const;

 private:
  oo::BaseId mBaseId;
  Ogre::TexturePtr mLowerCloudsTex;
  Ogre::TexturePtr mUpperCloudsTex;
  Ogre::MaterialPtr mSkyDomeMaterial;
  // TODO: Support rain

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

} // namespace oo

#endif // OPENOBLIVION_WTHR_RESOLVER_HPP
