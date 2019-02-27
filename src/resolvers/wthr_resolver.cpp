#include "conversions.hpp"
#include "record/records.hpp"
#include "resolvers/wthr_resolver.hpp"
#include "settings.hpp"
#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgreSceneManager.h>
#include <OgreTextureManager.h>

oo::Weather::Weather(const record::WTHR &rec) {
  mBaseId = oo::BaseId{rec.mFormId};
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  if (rec.lowerLayerFilename) {
    oo::Path basePath{"textures/" + rec.lowerLayerFilename->data};
    mLowerCloudsTex = texMgr.getByName(basePath.c_str(), oo::RESOURCE_GROUP);
  }
  if (rec.upperLayerFilename) {
    oo::Path basePath{"textures/" + rec.upperLayerFilename->data};
    mUpperCloudsTex = texMgr.getByName(basePath.c_str(), oo::RESOURCE_GROUP);
  }
  const auto &skyColors{rec.skyColors->data};

  constexpr auto sunrise{unsigned(oo::chrono::Sunrise)};
  mColors[sunrise].lowerSky = makeColor(skyColors.skyLower.sunrise);
  mColors[sunrise].upperSky = makeColor(skyColors.skyUpper.sunrise);
  mColors[sunrise].lowerClouds = makeColor(skyColors.cloudsLower.sunrise);
  mColors[sunrise].upperClouds = makeColor(skyColors.cloudsUpper.sunrise);
  mColors[sunrise].fog = makeColor(skyColors.fog.sunrise);
  mColors[sunrise].horizon = makeColor(skyColors.horizon.sunrise);
  mColors[sunrise].ambient = makeColor(skyColors.ambient.sunrise);
  mColors[sunrise].sun = makeColor(skyColors.sun.sunrise);
  mColors[sunrise].sunlight = makeColor(skyColors.sunlight.sunrise);
  mColors[sunrise].stars = makeColor(skyColors.stars.sunrise);

  constexpr auto daytime{unsigned(oo::chrono::Daytime)};
  mColors[daytime].lowerSky = makeColor(skyColors.skyLower.day);
  mColors[daytime].upperSky = makeColor(skyColors.skyUpper.day);
  mColors[daytime].lowerClouds = makeColor(skyColors.cloudsLower.day);
  mColors[daytime].upperClouds = makeColor(skyColors.cloudsUpper.day);
  mColors[daytime].fog = makeColor(skyColors.fog.day);
  mColors[daytime].horizon = makeColor(skyColors.horizon.day);
  mColors[daytime].ambient = makeColor(skyColors.ambient.day);
  mColors[daytime].sun = makeColor(skyColors.sun.day);
  mColors[daytime].sunlight = makeColor(skyColors.sunlight.day);
  mColors[daytime].stars = makeColor(skyColors.stars.day);

  constexpr auto sunset{unsigned(oo::chrono::Sunset)};
  mColors[sunset].lowerSky = makeColor(skyColors.skyLower.sunset);
  mColors[sunset].upperSky = makeColor(skyColors.skyUpper.sunset);
  mColors[sunset].lowerClouds = makeColor(skyColors.cloudsLower.sunset);
  mColors[sunset].upperClouds = makeColor(skyColors.cloudsUpper.sunset);
  mColors[sunset].fog = makeColor(skyColors.fog.sunset);
  mColors[sunset].horizon = makeColor(skyColors.horizon.sunset);
  mColors[sunset].ambient = makeColor(skyColors.ambient.sunset);
  mColors[sunset].sun = makeColor(skyColors.sun.sunset);
  mColors[sunset].sunlight = makeColor(skyColors.sunlight.sunset);
  mColors[sunset].stars = makeColor(skyColors.stars.sunset);

  constexpr auto nighttime{unsigned(oo::chrono::Nighttime)};
  mColors[nighttime].lowerSky = makeColor(skyColors.skyLower.night);
  mColors[nighttime].upperSky = makeColor(skyColors.skyUpper.night);
  mColors[nighttime].lowerClouds = makeColor(skyColors.cloudsLower.night);
  mColors[nighttime].upperClouds = makeColor(skyColors.cloudsUpper.night);
  mColors[nighttime].fog = makeColor(skyColors.fog.night);
  mColors[nighttime].horizon = makeColor(skyColors.horizon.night);
  mColors[nighttime].ambient = makeColor(skyColors.ambient.night);
  mColors[nighttime].sun = makeColor(skyColors.sun.night);
  mColors[nighttime].sunlight = makeColor(skyColors.sunlight.night);
  mColors[nighttime].stars = makeColor(skyColors.stars.night);

  if (rec.fogDistances) {
    mFogDistances[daytime] = FogDistance{
        rec.fogDistances->data.fogDayNear * oo::metersPerUnit<float>,
        rec.fogDistances->data.fogDayFar * oo::metersPerUnit<float>
    };
    mFogDistances[nighttime] = FogDistance{
        rec.fogDistances->data.fogNightNear * oo::metersPerUnit<float>,
        rec.fogDistances->data.fogNightFar * oo::metersPerUnit<float>
    };
    mFogDistances[sunrise] = mFogDistances[daytime];
    mFogDistances[sunset] = mFogDistances[nighttime];
  } else {
    mFogDistances.fill(FogDistance{1.0f, 2.0f});
  }
}

void oo::Weather::setSkyDome(Ogre::SceneManager *scnMgr) {
  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  if (!mSkyDomeMaterial) {
    auto matPtr{matMgr.getByName("__skyMaterial", oo::SHADER_GROUP)};
    mSkyDomeMaterial = matPtr->clone("__skyMaterial" + getBaseId().string(),
        /*changeGroup=*/true, oo::RESOURCE_GROUP);
    if (mLowerCloudsTex && mUpperCloudsTex) {
      Ogre::AliasTextureNamePairList layers{
          {"lowerLayer", mLowerCloudsTex->getName()},
          {"upperLayer", mUpperCloudsTex->getName()}
      };
      mSkyDomeMaterial->applyTextureAliases(layers, true);
    }
  }

  constexpr Ogre::Real curvature{10};
  constexpr Ogre::Real tiling{8};
  constexpr Ogre::Real distanceToPlane{4000};
  scnMgr->setSkyDome(true, mSkyDomeMaterial->getName(), curvature, tiling,
                     distanceToPlane, /*drawFirst=*/true,
                     Ogre::Quaternion::IDENTITY, 16, 16, -1,
                     oo::RESOURCE_GROUP);
}

void oo::Weather::setFog(Ogre::SceneManager *scnMgr,
                         chrono::QualitativeTimeOfDay tod) const {
  scnMgr->setFog(Ogre::FogMode::FOG_LINEAR, mColors[unsigned(tod)].fog, 0,
                 mFogDistances[unsigned(tod)].near,
                 mFogDistances[unsigned(tod)].far);
}

oo::BaseId oo::Weather::getBaseId() const noexcept {
  return mBaseId;
}

Ogre::MaterialPtr oo::Weather::getMaterial() const {
  return mSkyDomeMaterial;
}

Ogre::ColourValue oo::Weather::makeColor(record::raw::Color c) const noexcept {
  Ogre::ColourValue cv;
  cv.setAsABGR(c.v);
  return cv;
}

Ogre::ColourValue
oo::Weather::getAmbientColor(oo::chrono::QualitativeTimeOfDay tod) const {
  return mColors[unsigned(tod)].ambient;
}

Ogre::ColourValue
oo::Weather::getSunlightColor(oo::chrono::QualitativeTimeOfDay tod) const {
  return mColors[unsigned(tod)].sunlight;
}

Ogre::ColourValue
oo::Weather::getLowerSkyColor(chrono::QualitativeTimeOfDay tod) const {
  return mColors[unsigned(tod)].lowerSky;
}

Ogre::ColourValue
oo::Weather::getUpperSkyColor(chrono::QualitativeTimeOfDay tod) const {
  return mColors[unsigned(tod)].upperSky;
}

Ogre::ColourValue
oo::Weather::getLowerCloudColor(chrono::QualitativeTimeOfDay tod) const {
  return mColors[unsigned(tod)].lowerClouds;
}

Ogre::ColourValue
oo::Weather::getUpperCloudColor(chrono::QualitativeTimeOfDay tod) const {
  return mColors[unsigned(tod)].upperClouds;
}

Ogre::ColourValue
oo::Weather::getHorizonColor(chrono::QualitativeTimeOfDay tod) const {
  return mColors[unsigned(tod)].horizon;
}
