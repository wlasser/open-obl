#include "atmosphere.hpp"
#include "record/records.hpp"
#include "resolvers/wrld_resolver.hpp"
#include "settings.hpp"

namespace oo {

//===-------------------------------------------------------------------===//
// Weather functions
//===-------------------------------------------------------------------===//

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
    const auto matName{SKY_BASE_MATERIAL + getBaseId().string()};
    if (matMgr.resourceExists(matName, oo::RESOURCE_GROUP)) {
      mSkyMaterial = matMgr.getByName(matName, oo::RESOURCE_GROUP);
    } else {
      auto matPtr{matMgr.getByName(SKY_BASE_MATERIAL, oo::SHADER_GROUP)};
      mSkyMaterial = matPtr->clone(matName, /*changeGroup=*/true,
                                   oo::RESOURCE_GROUP);
    }
  }

  if (!mCloudsMaterial) {
    const auto matName{CLOUD_BASE_MATERIAL + getBaseId().string()};
    if (matMgr.resourceExists(matName, oo::RESOURCE_GROUP)) {
      mCloudsMaterial = matMgr.getByName(oo::RESOURCE_GROUP);
    } else {
      auto matPtr{matMgr.getByName(CLOUD_BASE_MATERIAL, oo::SHADER_GROUP)};
      mCloudsMaterial = matPtr->clone(matName, /*changeGroup=*/true,
                                      oo::RESOURCE_GROUP);
      if (mLowerCloudsTex && mUpperCloudsTex) {
        Ogre::AliasTextureNamePairList layers{
            {"lowerLayer", mLowerCloudsTex->getName()},
            {"upperLayer", mUpperCloudsTex->getName()}
        };
        mCloudsMaterial->applyTextureAliases(layers, true);
      }
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

//===-------------------------------------------------------------------===//
// Atmosphere functions
//===-------------------------------------------------------------------===//

Atmosphere::Atmosphere(oo::BaseId wrldId,
                       gsl::not_null<Ogre::SceneManager *> scnMgr,
                       Resolvers resolvers) : mScnMgr(scnMgr) {
  auto &wrldRes{oo::getResolver<record::WRLD>(resolvers)};
  auto &clmtRes{oo::getResolver<record::CLMT>(resolvers)};
  const record::WRLD &rec{*wrldRes.get(wrldId)};

  const auto clmtOpt = [&]() {
    if (rec.climate) return clmtRes.get(rec.climate->data);
    return clmtRes.get(oo::BaseId{0x00'00015f});
  }();

  if (!clmtOpt) return;
  const record::CLMT &clmt{*clmtOpt};

  makeClimateSettings(clmt);
  makeWeatherList(clmt, resolvers);
  makeSun(clmt);

//  const auto declination{getSunEquatorialCoordinates().declination};
//  setSunriseSunsetTimes(declination, Ogre::Degree(45.0f));

  if (!mWeathers.empty()) {
    mWeathers.front().setSkyDome(mScnMgr);
  }
}

Atmosphere::~Atmosphere() {
  mScnMgr->setSkyDomeEnabled(false);
  mScnMgr->setSkyBoxEnabled(false);
  if (mScnMgr->hasSceneNode(SUN_NODE)) {
    mScnMgr->destroySceneNode(SUN_NODE);
  }
  if (mScnMgr->hasLight(SUN_LIGHT)) {
    mScnMgr->destroyLight(SUN_LIGHT);
  }
  if (mScnMgr->hasBillboardSet(SUN_BILLBOARD_SET)) {
    mScnMgr->destroyBillboardSet(SUN_BILLBOARD_SET);
  }
}

void Atmosphere::makeClimateSettings(const record::CLMT &rec) {
  if (!rec.settings) return;
  const record::raw::TNAM_CLMT &settings{rec.settings->data};

  mSunriseBegin = oo::chrono::minutes{settings.sunriseBegin * 10};
  mSunriseEnd = oo::chrono::minutes{settings.sunriseEnd * 10};
  mSunsetBegin = oo::chrono::minutes{settings.sunsetBegin * 10};
  mSunsetEnd = oo::chrono::minutes{settings.sunsetEnd * 10};

  mVolatility = settings.volatility;

  mHasMasser = settings.hasMasser;
  mHasSecunda = settings.hasSecunda;
  mPhaseLength = settings.phaseLength;
}

void Atmosphere::makeWeatherList(const record::CLMT &rec, Resolvers resolvers) {
  if (!rec.weatherList) return;
  const auto &weathers{rec.weatherList->data.weathers};
  std::vector<uint32_t> chances;

  chances.reserve(weathers.size());
  mWeathers.reserve(weathers.size());

  auto &wthrRes{oo::getResolver<record::WTHR>(resolvers)};
  for (const auto &entry : weathers) {
    auto wthrOpt{wthrRes.get(entry.formId)};
    if (!wthrOpt) continue;
    mWeathers.emplace_back(*wthrOpt);
    chances.emplace_back(entry.chance);
  }

  mWeatherDistribution = WeatherDistribution(chances.begin(), chances.end());
}

void Atmosphere::makeSun(const record::CLMT &rec) {
  if (!rec.sunFilename) return;
  mHasSun = true;

  auto[sunNode, sunNodeCreated]{createOrRetrieveSunNode()};
  auto[sunLight, sunLightCreated]{createOrRetrieveSunLight()};
  if (sunLightCreated) sunNode->attachObject(sunLight);

  sunLight->setType(Ogre::Light::LightTypes::LT_DIRECTIONAL);
  sunNode->setDirection(0.0f, -1.0f, 0.0f,
                        Ogre::Node::TransformSpace::TS_WORLD);
  sunNode->setPosition(0.0f, oo::Weather::SUN_DISTANCE, 0.0f);

  auto[sunBillboards, sunBillboardsCreated]{createOrRetrieveSunBillboardSet()};
  if (sunBillboardsCreated) sunNode->attachObject(sunBillboards);

  auto[matPtr, matCreated]{createOrRetrieveSunMaterial(rec)};

  oo::Path sunFilename{"textures/" + rec.sunFilename->data};
  Ogre::AliasTextureNamePairList aliases{
      {"sunTexture", std::string{sunFilename.c_str()}}
  };
  matPtr->applyTextureAliases(aliases, true);

  sunBillboards->setMaterial(matPtr);
  sunBillboards->setPointRenderingEnabled(false);
  sunBillboards->setVisible(true);
}

std::pair<oo::chrono::QualitativeTimeOfDay, float>
Atmosphere::splitTime(const chrono::minutes &time) const noexcept {
  // I know the formatting is weird, I spent a while trying to make it readable

  // Sunrise
  if (mSunriseBegin <= time && time <= mSunriseEnd) {
    const auto sunriseMid{(mSunriseEnd + mSunriseBegin) / 2};
    const auto dt{static_cast<float>((sunriseMid - mSunriseBegin).count())};
    return time <= sunriseMid
           ? std::pair{chrono::Sunrise, (time - mSunriseBegin).count() / dt}
           : std::pair{chrono::Daytime, (time - sunriseMid).count() / dt};

    // Daytime
  } else if (mSunriseEnd < time && time < mSunsetBegin) {
    return {oo::chrono::Daytime, 1.0f};

    // Sunset
  } else if (mSunsetBegin <= time && time <= mSunsetEnd) {
    const auto sunsetMid{(mSunsetEnd + mSunsetBegin) / 2};
    const auto dt{static_cast<float>((sunsetMid - mSunsetBegin).count())};
    return time <= sunsetMid
           ? std::pair{chrono::Sunset, (time - mSunsetBegin).count() / dt}
           : std::pair{chrono::Nighttime, (time - sunsetMid).count() / dt};

    // Nighttime
  } else {
    return {oo::chrono::Nighttime, 1.0f};
  }
}

void Atmosphere::update(const oo::chrono::minutes &time) {
  auto *sunNode{getSunNode()};

  const auto domePos{mScnMgr->getSkyDomeNode()->getPosition()};
  const auto relPos{getSunPosition(time)};

  sunNode->setPosition(domePos + relPos * oo::Weather::SUN_DISTANCE);
  sunNode->setDirection(-relPos, Ogre::Node::TransformSpace::TS_WORLD);

  Ogre::ColourValue ambientCol{};
  Ogre::ColourValue sunlightCol{};
  Ogre::ColourValue sunCol{};

  auto[tod, t]{splitTime(time)};

  if (mWeathers.empty()) return;
  const auto &weather{mWeathers[mCurrentWeather]};

  ambientCol = weather.getAmbientColor(tod, t);
  sunlightCol = weather.getSunlightColor(tod, t);
  sunCol = weather.getSunColor(tod, t);

  Ogre::Light *light{getSunLight()};
  Ogre::Billboard *sunBillboard{getSunBillboard()};

  mScnMgr->setAmbientLight(ambientCol);
  if (light) light->setDiffuseColour(sunlightCol);
  if (sunBillboard) sunBillboard->setColour(sunCol);

  weather.setFog(mScnMgr, tod, t);
  weather.setSkyMaterial(tod, t);
}

//===----------------------------------------------------------------------===//
// Sun functions
//===----------------------------------------------------------------------===//

std::pair<Ogre::SceneNode *, bool> Atmosphere::createOrRetrieveSunNode() {
  if (mScnMgr->hasSceneNode(SUN_NODE)) {
    return {mScnMgr->getSceneNode(SUN_NODE), false};
  }
  auto *rootNode{mScnMgr->getRootSceneNode()};
  return {rootNode->createChildSceneNode(SUN_NODE), true};
}

std::pair<Ogre::Light *, bool> Atmosphere::createOrRetrieveSunLight() {
  if (mScnMgr->hasLight(SUN_LIGHT)) {
    return {mScnMgr->getLight(SUN_LIGHT), false};
  }
  return {mScnMgr->createLight(SUN_LIGHT), true};
}

std::pair<Ogre::BillboardSet *, bool>
Atmosphere::createOrRetrieveSunBillboardSet() {
  if (mScnMgr->hasBillboardSet(SUN_BILLBOARD_SET)) {
    return {mScnMgr->getBillboardSet(SUN_BILLBOARD_SET), false};
  }
  auto *set{mScnMgr->createBillboardSet(SUN_BILLBOARD_SET, 1)};

  set->setDefaultDimensions(oo::Weather::SUN_WIDTH, oo::Weather::SUN_WIDTH);
  set->setBillboardOrigin(Ogre::BillboardOrigin::BBO_CENTER);
  set->setBillboardRotationType(Ogre::BillboardRotationType::BBR_VERTEX);
  set->setBillboardType(Ogre::BillboardType::BBT_POINT);

  set->createBillboard(Ogre::Vector3::ZERO);

  return {set, true};
}

std::pair<Ogre::MaterialPtr, bool>
Atmosphere::createOrRetrieveSunMaterial(const record::CLMT &rec) {
  std::string matName{SUN_BASE_MATERIAL + oo::BaseId{rec.mFormId}.string()};
  auto &matMgr{Ogre::MaterialManager::getSingleton()};

  if (matMgr.resourceExists(matName, oo::RESOURCE_GROUP)) {
    return {matMgr.getByName(matName, oo::RESOURCE_GROUP), false};
  }
  auto basePtr{matMgr.getByName(SUN_BASE_MATERIAL, oo::SHADER_GROUP)};
  return {basePtr->clone(matName, /*changeGroup=*/true, oo::RESOURCE_GROUP),
          true};
}

Ogre::SceneNode *Atmosphere::getSunNode() const noexcept {
  if (mScnMgr->hasSceneNode(SUN_NODE)) {
    return mScnMgr->getSceneNode(SUN_NODE);
  }
  return nullptr;
}

Ogre::Light *Atmosphere::getSunLight() const noexcept {
  if (mScnMgr->hasLight(SUN_LIGHT)) {
    return mScnMgr->getLight(SUN_LIGHT);
  }
  return nullptr;
}

Ogre::Billboard *Atmosphere::getSunBillboard() const noexcept {
  if (mScnMgr->hasBillboardSet(SUN_BILLBOARD_SET)) {
    return mScnMgr->getBillboardSet(SUN_BILLBOARD_SET)->getBillboard(0);
  }
  return nullptr;
}

Ogre::Vector3
Atmosphere::getSunPositionSimple(const chrono::minutes &time) const {
  if (mSunriseBegin <= time && time <= mSunsetEnd) {
    auto dt{static_cast<float>((mSunsetEnd - mSunriseBegin).count())};
    Ogre::Radian theta{(time - mSunriseBegin).count() / dt * Ogre::Math::PI};
    return Ogre::Vector3{Ogre::Math::Cos(theta), Ogre::Math::Sin(theta), 0.0f};
  }
  return Ogre::Vector3::NEGATIVE_UNIT_Y;
}

Atmosphere::EquatorialCoordinates
Atmosphere::getSunEquatorialCoordinates() const {
  // No constexpr math functions, so we use statics to cache calculations.
  // TODO: Add constexpr to OGRE, in particular Ogre::Degree and Ogre::Radian

  // Eccentricity of the orbit is constant, use Earth's.
  constexpr float eccentricity{0.01671f};

  // Obliquity of the ecliptic is constant (ignoring precession), use Earth's.
  const static Ogre::Degree obliquity{23.4f};
  const static float cosObliquity{Ogre::Math::Cos(obliquity)};
  const static float sinObliquity{Ogre::Math::Sin(obliquity)};

  // No orbiting, so pick some mean longitude and mean anomaly.
  // TODO: Use actual calendar time to get the correct position instead of
  //       hardcoding the epoch.
  // These should be wrapped into [0, 360).
  const Ogre::Degree meanLongitude{std::fmod(280.5f + 138.0f, 360.0f)};
  const Ogre::Degree meanAnomaly{std::fmod(357.5f + 138.0f, 360.0f)};

  // Get the ecliptic coordinates of the sun.
  // Use the equation of the centre to find the actual ecliptic longitude,
  // the ecliptic longitude is effectively zero.
  // Only bother going to second order, since e^3 barely fits in a float.
  const static Ogre::Radian L1{2.0f * eccentricity};
  const static Ogre::Radian L2{5.0f / 4.0f * Ogre::Math::Pow(eccentricity, 2)};

  const Ogre::Degree eclipticLongitude{
      meanLongitude
          + Ogre::Degree(L1) * Ogre::Math::Sin(meanAnomaly)
          + Ogre::Degree(L2) * Ogre::Math::Sin(2 * meanAnomaly)};

  // Convert from ecliptic coordinates to equatorial coordinates.
  const Ogre::Radian rightAscension{Ogre::Math::ATan2(
      cosObliquity * Ogre::Math::Sin(eclipticLongitude),
      Ogre::Math::Cos(eclipticLongitude))};
  const Ogre::Radian declination{Ogre::Math::ASin(
      sinObliquity * Ogre::Math::Sin(eclipticLongitude)
  )};

  return {rightAscension, declination};
}

void Atmosphere::setSunriseSunsetTimes(const Ogre::Radian &declination,
                                       const Ogre::Radian &latitude) {
  const Ogre::Degree hourAngle{Ogre::Math::ACos(
      -Ogre::Math::Tan(declination) * Ogre::Math::Tan(latitude))};
  // Proportion of day from noon that sunrise/sunset occurs.
  // Our time is measured from midnight.
  const float t{hourAngle.valueDegrees() / 360.0f};
  const float sunriseBegin{(0.5f - t) * 24.0f * 60.0f};
  const float sunsetEnd{(0.5f + t) * 24.0f * 60.0f};

  // TODO: Fix these.
  const float sunriseEnd{sunriseBegin + 120.0f};
  const float sunsetBegin{sunsetEnd - 120.0f};

  spdlog::get(oo::LOG)->info("Sunrise t = {}", t);
  spdlog::get(oo::LOG)->info("Sunrise starts at {} and sunset ends at {}",
                             sunriseBegin / 60.0f, sunsetEnd / 60.0f);

  mSunriseBegin = chrono::minutes(static_cast<unsigned long>(sunriseBegin));
  mSunriseEnd = chrono::minutes(static_cast<unsigned long>(sunriseEnd));
  mSunsetBegin = chrono::minutes(static_cast<unsigned long>(sunsetBegin));
  mSunsetEnd = chrono::minutes(static_cast<unsigned long>(sunsetEnd));
}

Ogre::Vector3
Atmosphere::getSunPositionPhysical(const chrono::minutes &time) const {
  // Thanks to Wikipedia for all of these calculations.
  // https://en.wikipedia.org/Sunrise_equation
  // https://en.wikipedia.org/Position_of_the_Sun
  // https://en.wikipedia.org/Celestial_coordinate_system#Converting_coordinates
  const auto[rightAscension, declination]{getSunEquatorialCoordinates()};

  // Obviously we don't know the player's actual longitude and latitude.
  // The relative location and climate of the other continents on Nirn suggest
  // that Tamriel is in the northern hemisphere, and we may as well take the
  // prime meridian as going through the Imperial City.
  // Since we don't know the equatorial circumference of Nirn, we can't change
  // the longitude and latitude as the player moves anyway. Pick the player
  // as being on the prime meridian with a northern hemisphere latitude.
  const static Ogre::Degree latitude{45.0f};
  const float cosLatitude{Ogre::Math::Cos(latitude)};
  const float sinLatitude{Ogre::Math::Sin(latitude)};

  // For simplicity take sidereal time equal to solar time, implying also that
  // the observer is on the prime meridian.
  // Note 360 * t / (60 * 24) = t / 4
  const Ogre::Degree siderealTime{time.count() / 4.0f};
  const auto hourAngle{siderealTime - rightAscension};
  const float cosHourAngle{Ogre::Math::Cos(hourAngle)};
  const float sinHourAngle{Ogre::Math::Sin(hourAngle)};

  // Convert from equatorial coordinates to horizontal coordinates.
  const float cosDeclination{Ogre::Math::Cos(declination)};
  const float sinDeclination{Ogre::Math::Sin(declination)};

  // Azimuth is measured from South, turning positive Westward
  const Ogre::Radian azimuth{Ogre::Math::ATan2(
      sinHourAngle * cosDeclination,
      cosHourAngle * sinLatitude * cosDeclination - sinDeclination * cosLatitude
  )};

  // Altitude is measured from the horizon towards the zenith
  const Ogre::Radian altitude{Ogre::Math::ASin(
      sinLatitude * sinDeclination + cosLatitude * cosDeclination * cosHourAngle
  )};

  // Convert from horizontal polar coordinates to Cartesian coordinates.
  // X is East, Y is towards zenith, Z is North
  const float cosAltitude{Ogre::Math::Cos(altitude)};
  const float sinAltitude{Ogre::Math::Sin(altitude)};

  return Ogre::Vector3{
      -cosAltitude * Ogre::Math::Sin(azimuth),
      sinAltitude,
      cosAltitude * Ogre::Math::Cos(azimuth)
  };
}

Ogre::Vector3 Atmosphere::getSunPosition(const chrono::minutes &time) const {
  return getSunPositionSimple(time);
}

} // namespace oo