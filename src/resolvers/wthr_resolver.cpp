#include "conversions.hpp"
#include "record/records.hpp"
#include "resolvers/wthr_resolver.hpp"
#include "settings.hpp"
#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgrePass.h>
#include <OgreSceneManager.h>
#include <OgreTechnique.h>
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

  if (!mSkyMaterial) {
    auto matPtr{matMgr.getByName("__skyMaterial", oo::SHADER_GROUP)};
    mSkyMaterial = matPtr->clone("__skyMaterial" + getBaseId().string(),
        /*changeGroup=*/true, oo::RESOURCE_GROUP);
  }

  if (!mCloudsMaterial) {
    auto matPtr{matMgr.getByName("__cloudMaterial", oo::SHADER_GROUP)};
    mCloudsMaterial = matPtr->clone("__cloudMaterial" + getBaseId().string(),
        /*changeGroup=*/true, oo::RESOURCE_GROUP);
    if (mLowerCloudsTex && mUpperCloudsTex) {
      Ogre::AliasTextureNamePairList layers{
          {"lowerLayer", mLowerCloudsTex->getName()},
          {"upperLayer", mUpperCloudsTex->getName()}
      };
      mCloudsMaterial->applyTextureAliases(layers, true);
    }
  }

  constexpr Ogre::Real curvature{10};
  constexpr Ogre::Real tiling{8};
  scnMgr->setSkyDome(true, mCloudsMaterial->getName(), curvature, tiling,
                     oo::Weather::CLOUD_HEIGHT, /*drawFirst=*/false,
                     Ogre::Quaternion::IDENTITY, 16, 16, -1,
                     oo::RESOURCE_GROUP);

  scnMgr->setSkyBox(true, mSkyMaterial->getName(), oo::Weather::SKY_HEIGHT,
      /*drawFirst=*/true, Ogre::Quaternion::IDENTITY, oo::RESOURCE_GROUP);
}

void oo::Weather::setFog(Ogre::SceneManager *scnMgr,
                         chrono::QualitativeTimeOfDay tod,
                         float t) const {
  auto getNear = [this](chrono::QualitativeTimeOfDay tod) {
    return mFogDistances[unsigned(tod)].near;
  };
  auto getFar = [this](chrono::QualitativeTimeOfDay tod) {
    return mFogDistances[unsigned(tod)].far;
  };

  const float fogNear{interp<float>(tod, t, getNear)};
  const float fogFar{interp<float>(tod, t, getFar)};
  const auto fogColor{getColor(tod, t, [](const Colors &c) { return c.fog; })};

  scnMgr->setFog(Ogre::FogMode::FOG_LINEAR, fogColor, 0, fogNear, fogFar);
}

void oo::Weather::setSkyMaterial(oo::chrono::QualitativeTimeOfDay tod,
                                 float t) const {
  if (!mSkyMaterial) return;
  auto skyPass{mSkyMaterial->getTechnique(0)->getPass(0)};
  auto skyParams = [&skyPass]() -> Ogre::GpuProgramParametersSharedPtr {
    return skyPass ? skyPass->getFragmentProgramParameters()
                   : Ogre::GpuProgramParametersSharedPtr();
  }();
  if (!skyParams) return;

  skyParams->setNamedConstant("lowerSkyColor", getLowerSkyColor(tod, t));
  skyParams->setNamedConstant("upperSkyColor", getUpperSkyColor(tod, t));
  skyParams->setNamedConstant("horizonColor", getHorizonColor(tod, t));

  if (!mCloudsMaterial) return;
  auto cloudsPass{mCloudsMaterial->getTechnique(0)->getPass(0)};
  auto cloudsParams = [&cloudsPass]() -> Ogre::GpuProgramParametersSharedPtr {
    return cloudsPass ? cloudsPass->getFragmentProgramParameters()
                      : Ogre::GpuProgramParametersSharedPtr();
  }();
  if (!cloudsParams) return;

  cloudsParams->setNamedConstant("lowerCloudColor", getLowerCloudColor(tod, t));
  cloudsParams->setNamedConstant("upperCloudColor", getUpperCloudColor(tod, t));
}

oo::BaseId oo::Weather::getBaseId() const noexcept {
  return mBaseId;
}

Ogre::ColourValue oo::Weather::makeColor(record::raw::Color c) const noexcept {
  Ogre::ColourValue cv;
  cv.setAsABGR(c.v);
  return cv;
}

Ogre::ColourValue
oo::Weather::getAmbientColor(oo::chrono::QualitativeTimeOfDay tod,
                             float t) const {
  return getColor(tod, t, [](const Colors &c) { return c.ambient; });
}

Ogre::ColourValue
oo::Weather::getSunlightColor(oo::chrono::QualitativeTimeOfDay tod,
                              float t) const {
  return getColor(tod, t, [](const Colors &c) { return c.sunlight; });
}

Ogre::ColourValue
oo::Weather::getLowerSkyColor(chrono::QualitativeTimeOfDay tod,
                              float t) const {
  return getColor(tod, t, [](const Colors &c) { return c.lowerSky; });
}

Ogre::ColourValue
oo::Weather::getUpperSkyColor(chrono::QualitativeTimeOfDay tod,
                              float t) const {
  return getColor(tod, t, [](const Colors &c) { return c.upperSky; });
}

Ogre::ColourValue
oo::Weather::getLowerCloudColor(chrono::QualitativeTimeOfDay tod,
                                float t) const {
  return getColor(tod, t, [](const Colors &c) { return c.lowerClouds; });
}

Ogre::ColourValue
oo::Weather::getUpperCloudColor(chrono::QualitativeTimeOfDay tod,
                                float t) const {
  return getColor(tod, t, [](const Colors &c) { return c.upperClouds; });
}

Ogre::ColourValue
oo::Weather::getSunColor(chrono::QualitativeTimeOfDay tod, float t) const {
  return getColor(tod, t, [](const Colors &c) { return c.sun; });
}

Ogre::ColourValue
oo::Weather::getHorizonColor(chrono::QualitativeTimeOfDay tod,
                             float t) const {
  return getColor(tod, t, [](const Colors &c) { return c.horizon; });
}
