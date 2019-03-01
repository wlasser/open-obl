#include "esp.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include "settings.hpp"
#include <OgrePixelFormat.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <Terrain/OgreTerrainGroup.h>

//===----------------------------------------------------------------------===//
// WRLD resolver definitions
//===----------------------------------------------------------------------===//

std::pair<oo::Resolver<record::WRLD>::RecordIterator, bool>
oo::Resolver<record::WRLD>::insertOrAppend(oo::BaseId baseId,
                                           const record::WRLD &rec,
                                           oo::EspAccessor accessor) {
  spdlog::get(oo::LOG)->info("Inserting WRLD {}, {}", baseId,
                             rec.editorId.data);

  RecordEntry entry{std::make_pair(rec, tl::nullopt)};
  Metadata meta{{accessor}, {}};
  auto[it, inserted]{mRecords.try_emplace(baseId, entry, meta)};
  if (inserted) return {it, inserted};

  auto &wrappedEntry{it->second};
  wrappedEntry.second.mAccessors.push_back(accessor);
  wrappedEntry.first = std::make_pair(rec, tl::nullopt);

  return {it, inserted};
}

tl::optional<const record::WRLD &>
oo::Resolver<record::WRLD>::get(oo::BaseId baseId) const {
  const auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  const auto &entry{it->second.first};

  return entry.second ? *entry.second : entry.first;
}

/// \overload get(oo::BaseId)
tl::optional<record::WRLD &>
oo::Resolver<record::WRLD>::get(oo::BaseId baseId) {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  auto &entry{it->second.first};

  if (!entry.second) entry.second.emplace(entry.first);
  return *entry.second;
}

/// Check if there is a world with the baseId.
bool oo::Resolver<record::WRLD>::contains(oo::BaseId baseId) const {
  return mRecords.find(baseId) != mRecords.end();
}

void oo::Resolver<record::WRLD>::load(oo::BaseId baseId,
                                      BaseResolverContext baseCtx) {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return;
  auto &meta{it->second.second};
  meta.mCells.clear();

  WrldVisitor visitor(meta, baseCtx);
  // Taking accessors by value so subsequent reads will work
  for (auto accessor : meta.mAccessors) {
    oo::readWrldChildren(accessor, visitor);
  }
}

tl::optional<const absl::flat_hash_set<oo::BaseId> &>
oo::Resolver<record::WRLD>::getCells(oo::BaseId baseId) const {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  return it->second.second.mCells;
}

template<> void
oo::Resolver<record::WRLD>::WrldVisitor::readRecord<record::CELL>(oo::EspAccessor &accessor) {
  auto &cellRes{oo::getResolver<record::CELL>(mBaseCtx)};
  const auto rec{accessor.readRecord<record::CELL>().value};
  const oo::BaseId baseId{rec.mFormId};
  cellRes.insertOrAppend(baseId, rec, accessor, /*isExterior=*/true);
  mMeta.mCells.emplace(baseId);
  if (accessor.peekGroupType() == record::Group::GroupType::CellChildren) {
    accessor.skipGroup();
  }
}

//===----------------------------------------------------------------------===//
// World definitions
//===----------------------------------------------------------------------===//

oo::BaseId oo::World::getBaseId() const {
  return mBaseId;
}

std::string oo::World::getName() const {
  return mName;
}

void oo::World::setName(std::string name) {
  mName = std::move(name);
}

gsl::not_null<Ogre::SceneManager *> oo::World::getSceneManager() const {
  return mScnMgr;
}

gsl::not_null<oo::World::PhysicsWorld *> oo::World::getPhysicsWorld() const {
  return gsl::make_not_null(mPhysicsWorld.get());
}

oo::World::CellIndex oo::World::getCellIndex(float x, float y) const {
  return {
      static_cast<int32_t>(std::floor(x / oo::unitsPerCell<float>)),
      static_cast<int32_t>(std::floor(y / oo::unitsPerCell<float>))
  };
}

void oo::World::setDefaultImportData() {
  auto &importData{mTerrainGroup.getDefaultImportSettings()};
  importData.constantHeight = 0.0f;
  importData.inputFloat = nullptr;
  importData.deleteInputData = true;
  importData.inputImage = nullptr;
  importData.terrainSize = oo::verticesPerQuad<uint16_t>;
  importData.terrainAlign = Ogre::Terrain::Alignment::ALIGN_X_Z;
  importData.worldSize = oo::metersPerUnit<float> * oo::unitsPerQuad<float>;
  importData.maxBatchSize = oo::verticesPerQuad<uint16_t>;
  importData.minBatchSize = static_cast<uint16_t>(
      oo::verticesPerQuad<uint16_t> / 2u + 1u);
}

oo::World::World(oo::BaseId baseId, std::string name, Resolvers resolvers)
    : mBaseId(baseId), mName(std::move(name)),
      mScnMgr(Ogre::Root::getSingleton().createSceneManager()),
      mTerrainGroup(mScnMgr, Ogre::Terrain::Alignment::ALIGN_X_Z,
                    oo::verticesPerQuad<uint16_t>,
                    oo::metersPerUnit<Ogre::Real> * oo::unitsPerQuad<float>),
      mResolvers(std::move(resolvers)) {
  // Shift origin because cell coordinates give SW corner position but Ogre
  // works with the centre.
  mTerrainGroup.setOrigin(oo::fromBSCoordinates(Ogre::Vector3{
      oo::unitsPerQuad<float> / 2.0f,
      oo::unitsPerQuad<float> / 2.0f,
      0.0f
  }));
  mTerrainGroup.setResourceGroup(oo::RESOURCE_GROUP);
  setDefaultImportData();

  makePhysicsWorld();
  makeCellGrid();
  makeAtmosphere();
}

oo::World::~World() {
  mTerrainGroup.removeAllTerrains();
  auto root{Ogre::Root::getSingletonPtr()};
  if (root) root->destroySceneManager(mScnMgr);
}

void oo::World::makeCellGrid() {
  const auto &wrldRes{oo::getResolver<record::WRLD>(mResolvers)};
  const record::WRLD &rec{*wrldRes.get(mBaseId)};

  // Worldspace bounds, in units.
  const auto[x0, y0]{rec.bottomLeft.data};
  const auto[x1, y1]{rec.topRight.data};

  // Worldspace bounds, in cells.
  const auto &p0{getCellIndex(x0, y0)};
  const auto &p1{getCellIndex(x1, y1)};

  mCells.resize(boost::extents[qvm::X(p1 - p0) + 1u][qvm::Y(p1 - p0) + 1u]);
  mCells.reindex(std::array{qvm::X(p0), qvm::Y(p0)});

  // Need non-const CELL and LAND for loadTerrain, then const LAND.
  auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};
  const auto &landRes{oo::getResolver<record::LAND>(mResolvers)};
  const auto &ltexRes{oo::getResolver<record::LTEX>(mResolvers)};
  for (auto cellId : *wrldRes.getCells(mBaseId)) {
    const auto cellOpt{cellRes.get(cellId)};
    if (!cellOpt) continue;

    const auto gridOpt{cellOpt->grid};
    if (!gridOpt) continue;

    const auto grid{gridOpt->data};
    CellIndex p{grid.x, grid.y};
    mCells[qvm::X(p)][qvm::Y(p)] = cellId;

    cellRes.loadTerrain(cellId, oo::getResolvers<record::LAND>(mResolvers));
    const auto landId{cellRes.getLandId(cellId)};
    if (!landId) continue; // TODO: Is this an error?

    const auto landOpt{landRes.get(*landId)};
    if (!landOpt) continue;

    if (!landOpt->heights) continue;
    const record::raw::VHGT &heightRec{landOpt->heights->data};

    // NB: If you want to change this to a standard ImportData() constructor
    // then make sure its terrainSize is set correctly---even though the
    // TerrainGroup knows it already---otherwise each defineTerrain will copy
    // 4MB of data for inputFloat and promptly OOM your machine when the main
    // worldspace loads.
    ImportDataArray importData;
    importData.fill(mTerrainGroup.getDefaultImportSettings());

    setTerrainHeights(heightRec, importData);

    auto &terrainOpts{Ogre::TerrainGlobalOptions::getSingleton()};
    const auto matGen{terrainOpts.getDefaultMaterialGenerator()};
    for (auto &data : importData) {
      data.layerDeclaration = matGen->getLayerDeclaration();
    }

    auto layerOrders{makeDefaultLayerOrders(*landOpt)};
    applyBaseLayers(layerOrders, *landOpt);
    applyFineLayers(layerOrders, *landOpt);

    for (std::size_t i = 0; i < 4; ++i) {
      for (auto id : layerOrders[i]) {
        auto &layer{importData[i].layerList.emplace_back()};
        layer.worldSize = 1.0f;

        if (const auto ltexOpt{ltexRes.get(id)}) {
          const oo::Path basePath{ltexOpt->textureFilename.data};
          emplaceTexture(layer.textureNames, basePath.c_str());
        } else {
          emplaceTexture(layer.textureNames, "terrainhddirt01.dds");
        }
      }
    }

    const auto x{qvm::X(p)};
    const auto y{qvm::Y(p)};
    mTerrainGroup.defineTerrain(2 * x + 0, 2 * y + 0, &importData[0]);
    mTerrainGroup.defineTerrain(2 * x + 1, 2 * y + 0, &importData[1]);
    mTerrainGroup.defineTerrain(2 * x + 0, 2 * y + 1, &importData[2]);
    mTerrainGroup.defineTerrain(2 * x + 1, 2 * y + 1, &importData[3]);
  }
}

void oo::World::makePhysicsWorld() {
  const auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};
  const auto &bulletConf{cellRes.getBulletConfiguration()};
  mPhysicsWorld = bulletConf.makeDynamicsWorld();
}

void oo::World::makeAtmosphere() {
  auto &wrldRes{oo::getResolver<record::WRLD>(mResolvers)};
  auto &clmtRes{oo::getResolver<record::CLMT>(mResolvers)};
  auto &wthrRes{oo::getResolver<record::WTHR>(mResolvers)};
  const record::WRLD &rec{*wrldRes.get(mBaseId)};

  const auto clmtOpt = [&]() {
    if (rec.climate) return clmtRes.get(rec.climate->data);
    return clmtRes.get(oo::BaseId{0x00'00015f});
  }();

  if (!clmtOpt) return;
  const record::CLMT &clmt{*clmtOpt};

  if (clmt.settings) {
    const record::raw::TNAM_CLMT &settings{clmt.settings->data};

    mSunriseBegin = oo::chrono::minutes{settings.sunriseBegin * 10};
    mSunriseEnd = oo::chrono::minutes{settings.sunriseEnd * 10};
    mSunsetBegin = oo::chrono::minutes{settings.sunsetBegin * 10};
    mSunsetEnd = oo::chrono::minutes{settings.sunsetEnd * 10};

    mVolatility = settings.volatility;

    mHasMasser = settings.hasMasser;
    mHasSecunda = settings.hasSecunda;
    mPhaseLength = settings.phaseLength;
  }

  if (clmt.weatherList) {
    const auto &weathers{clmt.weatherList->data.weathers};
    std::vector<uint32_t> chances;

    chances.reserve(chances.size());
    mWeathers.reserve(weathers.size());

    for (const auto &entry : clmt.weatherList->data.weathers) {
      auto wthrOpt{wthrRes.get(entry.formId)};
      if (!wthrOpt) continue;
      mWeathers.emplace_back(*wthrOpt);
      chances.emplace_back(entry.chance);
    }

    mWeatherDistribution = std::discrete_distribution(chances.begin(),
                                                      chances.end());

    if (mWeathers.empty()) return;
    auto &weather{mWeathers[mCurrentWeather]};
    weather.setSkyDome(mScnMgr);
  }

  if (clmt.sunFilename) {
    mHasSun = true;
    auto *sunNode
        {mScnMgr->getRootSceneNode()->createChildSceneNode("__sunNode")};
    auto *sunLight{mScnMgr->createLight("__sunLight")};
    sunNode->attachObject(sunLight);

    sunNode->setDirection(0.0f, -1.0f, 0.0f,
                          Ogre::Node::TransformSpace::TS_WORLD);

    auto *sunBillboardSet{mScnMgr->createBillboardSet("__sunBillboardSet", 1)};
    sunNode->attachObject(sunBillboardSet);

    sunBillboardSet->setDefaultDimensions(oo::Weather::SUN_WIDTH,
                                          oo::Weather::SUN_WIDTH);
    sunBillboardSet->setBillboardOrigin(Ogre::BillboardOrigin::BBO_CENTER);
    sunBillboardSet->setBillboardRotationType(
        Ogre::BillboardRotationType::BBR_VERTEX);
    sunBillboardSet->setBillboardType(Ogre::BillboardType::BBT_POINT);

    std::string matName = "__sunMaterial" + oo::BaseId{clmt.mFormId}.string();
    auto matPtr = [&]() {
      auto &matMgr{Ogre::MaterialManager::getSingleton()};
      if (matMgr.resourceExists(matName, oo::RESOURCE_GROUP)) {
        return matMgr.getByName(matName, oo::RESOURCE_GROUP);
      }
      auto basePtr{matMgr.getByName("__sunMaterial", oo::SHADER_GROUP)};
      return basePtr->clone(matName, /*changeGroup=*/true, oo::RESOURCE_GROUP);
    }();

    oo::Path sunFilename{"textures/" + clmt.sunFilename->data};

    Ogre::AliasTextureNamePairList aliases{
        {"sunTexture", std::string{sunFilename.c_str()}}
    };
    matPtr->applyTextureAliases(aliases, true);

    sunBillboardSet->setMaterial(matPtr);
    sunBillboardSet->setPointRenderingEnabled(false);
    sunBillboardSet->setVisible(true);

    auto *sunBillboard{sunBillboardSet->createBillboard(Ogre::Vector3::ZERO)};
    if (!sunBillboard) {
      spdlog::get(oo::LOG)->warn("Failed to create sun billboard");
    }

    sunLight->setType(Ogre::Light::LightTypes::LT_DIRECTIONAL);
  }
}

void oo::World::updateAtmosphere(const oo::chrono::minutes &time) {
  if (mWeathers.empty()) return;
  const auto &weather{mWeathers[mCurrentWeather]};

  // getLight() throws if the light doesn't exist instead of returning nullptr.
  Ogre::Light *light = [&]() -> Ogre::Light * {
    if (mScnMgr->hasLight("__sunLight")) {
      return mScnMgr->getLight("__sunLight");
    }
    return nullptr;
  }();

  Ogre::Billboard *sunBillboard = [&]() -> Ogre::Billboard * {
    auto *set{mScnMgr->getBillboardSet("__sunBillboardSet")};
    if (!set) return nullptr;
    return set->getBillboard(0);
  }();

  if (mSunriseBegin <= time && time <= mSunriseEnd) {
    // Sunrise
    const auto sunriseMid{(mSunriseEnd + mSunriseBegin) / 2};
    const auto dt{static_cast<float>((sunriseMid - mSunriseBegin).count())};

    if (time <= sunriseMid) {
      // Blending into middle of sunrise from nighttime
      const float t{(time - mSunriseBegin).count() / dt};

      const auto c{weather.getAmbientColor(oo::chrono::Sunrise, t)};
      const auto sl{weather.getSunlightColor(oo::chrono::Sunrise, t)};
      const auto s{weather.getSunColor(oo::chrono::Sunrise, t)};

      weather.setFog(mScnMgr, oo::chrono::Sunrise, t);
      weather.setSkyMaterial(oo::chrono::Sunrise, t);
      mScnMgr->setAmbientLight(c);
      if (light) light->setDiffuseColour(sl);
      if (sunBillboard) sunBillboard->setColour(s);
    } else {
      // Blending into daytime from middle of sunrise
      const float t{(time - sunriseMid).count() / dt};

      const auto c{weather.getAmbientColor(oo::chrono::Daytime, t)};
      const auto sl{weather.getSunlightColor(oo::chrono::Daytime, t)};
      const auto s{weather.getSunColor(oo::chrono::Daytime, t)};

      weather.setFog(mScnMgr, oo::chrono::Daytime, t);
      weather.setSkyMaterial(oo::chrono::Daytime, t);
      mScnMgr->setAmbientLight(c);
      if (light) light->setDiffuseColour(sl);
      if (sunBillboard) sunBillboard->setColour(s);
    }
  } else if (mSunriseEnd < time && time < mSunsetBegin) {
    // Daytime
    const auto c{weather.getAmbientColor(oo::chrono::Daytime)};
    const auto sl{weather.getSunlightColor(oo::chrono::Daytime)};
    const auto s{weather.getSunColor(oo::chrono::Daytime)};

    weather.setFog(mScnMgr, oo::chrono::Daytime);
    weather.setSkyMaterial(oo::chrono::Daytime);
    mScnMgr->setAmbientLight(c);
    if (light) light->setDiffuseColour(sl);
    if (sunBillboard) sunBillboard->setColour(s);
  } else if (mSunsetBegin <= time && time <= mSunsetEnd) {
    // Sunset
    const auto sunsetMid{(mSunsetEnd + mSunsetBegin) / 2};
    const auto dt{static_cast<float>((sunsetMid - mSunsetBegin).count())};

    if (time <= sunsetMid) {
      // Blending into middle of sunset from daytime
      const float t{(time - mSunsetBegin).count() / dt};

      const auto c{weather.getAmbientColor(oo::chrono::Sunset, t)};
      const auto sl{weather.getSunlightColor(oo::chrono::Sunset, t)};
      const auto s{weather.getSunColor(oo::chrono::Sunset, t)};

      weather.setFog(mScnMgr, oo::chrono::Sunset, t);
      weather.setSkyMaterial(oo::chrono::Sunset, t);
      mScnMgr->setAmbientLight(c);
      if (light) light->setDiffuseColour(sl);
      if (sunBillboard) sunBillboard->setColour(s);
    } else {
      // Blending into nighttime from middle of sunset
      const float t{(time - sunsetMid).count() / dt};

      const auto c{weather.getAmbientColor(oo::chrono::Nighttime, t)};
      const auto sl{weather.getSunlightColor(oo::chrono::Nighttime, t)};
      const auto s{weather.getSunColor(oo::chrono::Nighttime, t)};

      weather.setFog(mScnMgr, oo::chrono::Nighttime, t);
      weather.setSkyMaterial(oo::chrono::Nighttime, t);
      mScnMgr->setAmbientLight(c);
      if (light) light->setDiffuseColour(sl);
      if (sunBillboard) sunBillboard->setColour(s);
    }
  } else {
    // Nighttime
    const auto c{weather.getAmbientColor(oo::chrono::Nighttime)};
    const auto sl{weather.getSunlightColor(oo::chrono::Nighttime)};
    const auto s{weather.getSunColor(oo::chrono::Nighttime)};

    weather.setFog(mScnMgr, oo::chrono::Nighttime);
    weather.setSkyMaterial(oo::chrono::Nighttime);
    mScnMgr->setAmbientLight(c);
    if (light) light->setDiffuseColour(sl);
    if (sunBillboard) sunBillboard->setColour(s);
  }

  auto *sunNode{mScnMgr->getSceneNode("__sunNode")};

  const auto domePos{mScnMgr->getSkyDomeNode()->getPosition()};
//  const auto relPos = [&]() -> Ogre::Vector3 {
//    if (mSunriseBegin <= time && time <= mSunsetEnd) {
//      auto dt{static_cast<float>((mSunsetEnd - mSunriseBegin).count())};
//      Ogre::Radian theta{(time - mSunriseBegin).count() / dt * Ogre::Math::PI};
//      return Ogre::Vector3{Ogre::Math::Cos(theta), Ogre::Math::Sin(theta), 0.0f}
//          * oo::Weather::SUN_DISTANCE;
//    }
//    return Ogre::Vector3::NEGATIVE_UNIT_Y * oo::Weather::SUN_DISTANCE;
//  }();
  const auto relPos = [&]() -> Ogre::Vector3 {
    // Thanks to Wikipedia for all of these calculations.
    // https://en.wikipedia.org/Sunrise_equation
    // https://en.wikipedia.org/Position_of_the_Sun
    // https://en.wikipedia.org/Celestial_coordinate_system#Converting_coordinates

    // No orbiting, so pick some mean longitude and mean anomaly
    // TODO: Use actual calendar time to get the correct position instead of
    //       hardcoding the epoch.
    // These should be wrapped into [0, 360).
    Ogre::Degree meanLongitude{std::fmod(280.5f + 238.0f, 360.0f)};
    Ogre::Degree meanAnomaly{std::fmod(357.5f + 238.0f, 360.0f)};

    // Eccentricity of the orbit is constant
    constexpr float eccentricity{0.01671f};
    // Semimajor axis is constant, measured in (our version of) AU.
    constexpr float semiMajorAxis{1.0f};
    constexpr float semiLatusRectum{semiMajorAxis
                                        * (1.0f - eccentricity * eccentricity)};
    // Obliquity of the ecliptic is constant (ignoring precession)
    // TODO: Add constexpr to OGRE
    const Ogre::Degree obliquity{23.4f};

    // Use the equation of the centre to find the actual ecliptic longitude
    Ogre::Radian L1{2.0f * eccentricity
                        - 0.25f * Ogre::Math::Pow(eccentricity, 3)};
    Ogre::Radian L2{5.0f / 4.0f * Ogre::Math::Pow(eccentricity, 2)
                        - 11.0f / 24.0f * Ogre::Math::Pow(eccentricity, 4)};
    Ogre::Degree eclipticLongitude{
        meanLongitude + Ogre::Degree(L1) * Ogre::Math::Sin(meanAnomaly)
            + Ogre::Degree(L2) * Ogre::Math::Sin(2 * meanAnomaly)};

    // Ecliptic latitude of the sun is approximately zero
    Ogre::Degree eclipticLatitude{0.0f};

    // Distance to the sun using polar coordinates
    // This is fudged because we don't actually place the sun that far away
    // and we only need it for final positioning
    float r = oo::Weather::SUN_DISTANCE;
//    float r = semiLatusRectum / (1.0f + Ogre::Math::Cos(eclipticLongitude));

    // Compute position in equatorial coordinates
    Ogre::Radian rightAscension{Ogre::Math::ATan2(
        Ogre::Math::Cos(obliquity) * Ogre::Math::Sin(eclipticLongitude),
        Ogre::Math::Cos(eclipticLongitude))};
    Ogre::Radian declination{Ogre::Math::ASin(
        Ogre::Math::Sin(obliquity) * Ogre::Math::Sin(eclipticLongitude)
    )};

    // Obviously we don't know the player's actual longitude and latitude.
    // The relative location and climate of the other continents on Nirn suggest
    // that Tamriel is in the northern hemisphere, and we may as well take the
    // prime meridian as going through the Imperial City.
    // Since we don't know the equatorial circumference of Nirn, we can't change
    // the longitude and latitude anyway so just pick a value.

    // For simplicity take sidereal time equal to solar time.
    // Note 360 * t / (60 * 24) = t / 4
    Ogre::Degree siderealTime{time.count() / 4.0f};
    auto hourAngle{siderealTime - rightAscension};
    // Observer's latitude
    Ogre::Degree latitude{0.0f};

    // Compute position in horizontal coordinates
    const float cosDeclination{Ogre::Math::Cos(declination)};
    const float sinDeclination{Ogre::Math::Sin(declination)};
    const float cosLatitude{Ogre::Math::Cos(latitude)};
    const float sinLatitude{Ogre::Math::Sin(latitude)};

    // Azimuth is measured from South, turning positive Westward
    Ogre::Radian azimuth{Ogre::Math::ATan2(
        Ogre::Math::Sin(hourAngle) * cosDeclination,
        Ogre::Math::Cos(hourAngle) * sinLatitude * cosDeclination
            - sinDeclination * cosLatitude
    )};

    // Altitude is measured from the horizon towards the zenith
    Ogre::Radian altitude{Ogre::Math::ASin(
        sinLatitude * sinDeclination
            + cosLatitude * cosDeclination * Ogre::Math::Cos(hourAngle)
    )};

    // X is East, Y is towards zenith, Z is North
    return Ogre::Vector3{
        -Ogre::Math::Cos(altitude) * Ogre::Math::Sin(azimuth),
        Ogre::Math::Sin(altitude),
        Ogre::Math::Cos(altitude) * Ogre::Math::Cos(azimuth)
    } * oo::Weather::SUN_DISTANCE;
  }();

  sunNode->setPosition(domePos + relPos);
  sunNode->setDirection(-relPos, Ogre::Node::TransformSpace::TS_WORLD);
}

void oo::World::setTerrainHeights(const record::raw::VHGT &rec,
                                  ImportDataArray &importData) const {
  constexpr auto vpc{oo::verticesPerCell<std::size_t>};
  constexpr auto vpq{oo::verticesPerQuad<std::size_t>};

  // Allocation method required for Ogre to manage the memory and delete it.
  for (auto &data : importData) {
    data.inputFloat = OGRE_ALLOC_T(float, vpq * vpq,
                                   Ogre::MEMCATEGORY_GEOMETRY);
  }

  // The height data is given as offsets. Moving to the right increases the
  // offset by the height value, moving to a new row resets it to the height
  // of the first value on the row before.
  const float scale{record::raw::VHGT::MULTIPLIER * oo::metersPerUnit<float>};
  float rowStartHeight{rec.offset * scale};

  // Because of the offsets its much easier to treat the entire cell as a whole,
  // then pull out the quadrants afterwards.
  std::array<float, vpc * vpc> tmp{};

  for (std::size_t j = 0; j < vpc; ++j) {
    const std::size_t o{j * vpc};

    rowStartHeight += rec.heights[o] * scale;
    tmp[o] = rowStartHeight;

    float height{rowStartHeight};
    for (std::size_t i = 1; i < vpc; ++i) {
      height += rec.heights[o + i] * scale;
      tmp[o + i] = height;
    }
  }

  for (std::size_t j = 0; j < vpq; ++j) {
    std::memcpy(&importData[0].inputFloat[vpq * j],
                &tmp[vpc * j],
                vpq * sizeof(float));
  }

  for (std::size_t j = 0; j < vpq; ++j) {
    std::memcpy(&importData[1].inputFloat[vpq * j],
                &tmp[vpc * j + (vpq - 1u)],
                vpq * sizeof(float));
  }

  for (std::size_t j = 0; j < vpq; ++j) {
    std::memcpy(&importData[2].inputFloat[vpq * j],
                &tmp[vpc * (j + vpq - 1u)],
                vpq * sizeof(float));
  }

  for (std::size_t j = 0; j < vpq; ++j) {
    std::memcpy(&importData[3].inputFloat[vpq * j],
                &tmp[vpc * (j + vpq - 1u) + (vpq - 1u)],
                vpq * sizeof(float));
  }
}

void
oo::World::emplaceTexture(Ogre::StringVector &list, std::string texName) const {
  std::string fullName{"textures/landscape/" + std::move(texName)};
  list.emplace_back(fullName);
  fullName.replace(fullName.begin() + fullName.find_last_of('.'),
                   fullName.end(), "_n.dds");

  auto &texMgr{Ogre::TextureManager::getSingleton()};
  if (texMgr.resourceExists(fullName, oo::RESOURCE_GROUP)) {
    list.emplace_back(std::move(fullName));
  } else {
    list.emplace_back("textures/flat_n.dds");
  }
}

oo::World::LayerMaps
oo::World::makeDefaultLayerMaps(const record::LAND &rec) const {
  LayerMaps layerMaps;
  std::generate(layerMaps.begin(), layerMaps.end(), []() -> LayerMap {
    LayerMap layers{};
    layers[oo::BaseId{0}].fill(255);
    return layers;
  });

  return layerMaps;
}

oo::World::LayerOrders
oo::World::makeDefaultLayerOrders(const record::LAND &rec) const {
  LayerOrders layerOrders;
  std::generate(layerOrders.begin(), layerOrders.end(), []() -> LayerOrder {
    return std::vector{oo::BaseId{0}};
  });

  return layerOrders;
}

void oo::World::applyBaseLayers(LayerMaps &layerMaps,
                                const record::LAND &rec) const {
  // Find all the quadrant base textures, overwriting the default layer.
  for (const record::BTXT &quadrantTexture : rec.quadrantTexture) {
    const int quadrant{quadrantTexture.data.quadrant};
    oo::BaseId id{quadrantTexture.data.id};
    layerMaps[quadrant].clear();
    layerMaps[quadrant][id].fill(255);
  }
}

void oo::World::applyBaseLayers(LayerOrders &layerOrders,
                                const record::LAND &rec) const {
  // Find all the quadrant base textures, overwriting the default layer.
  for (const record::BTXT &quadrantTexture : rec.quadrantTexture) {
    const int quadrant{quadrantTexture.data.quadrant};
    oo::BaseId id{quadrantTexture.data.id};
    layerOrders[quadrant][0] = id;
  }
}

void oo::World::applyFineLayers(LayerMaps &layerMaps,
                                const record::LAND &rec) const {
  // Find all the quadrant layer textures
  for (const auto &[atxt, vtxt] : rec.fineTextures) {
    const oo::BaseId id{atxt.data.id};
    const std::size_t quadrant{atxt.data.quadrant};

    // Record the layer texture for this quadrant
    auto &blendMap{layerMaps[quadrant][id]};
    for (auto &point : vtxt.data.points) {
      blendMap[point.position] = static_cast<uint8_t>(point.opacity * 255);
    }
  }
}

void oo::World::applyFineLayers(LayerOrders &layerOrders,
                                const record::LAND &rec) const {
  // Find all the quadrant layer textures
  for (const auto &[atxt, vtxt] : rec.fineTextures) {
    const oo::BaseId id{atxt.data.id};
    const std::size_t quadrant{atxt.data.quadrant};
    // ATXT layer index ignores the base layer, so is off by one.
    const std::size_t textureLayer{atxt.data.textureLayer + 1u};

    // Record the layer texture for this quadrant
    auto &order{layerOrders[quadrant]};
    if (order.size() <= textureLayer) order.resize(textureLayer + 1u);
    order[textureLayer] = id;
  }
}

oo::BaseId oo::World::getCell(CellIndex index) const {
  return mCells[qvm::X(index)][qvm::Y(index)];
}

void oo::World::loadTerrain(CellIndex index, bool async) {
  // TODO: Defer the loading of the collision mesh so this can be async
  auto x{qvm::X(index)};
  auto y{qvm::Y(index)};
  mTerrainGroup.loadTerrain(2 * x + 0, 2 * y + 0, !async);
  mTerrainGroup.loadTerrain(2 * x + 1, 2 * y + 0, !async);
  mTerrainGroup.loadTerrain(2 * x + 0, 2 * y + 1, !async);
  mTerrainGroup.loadTerrain(2 * x + 1, 2 * y + 1, !async);
}

void oo::World::loadTerrainOnly(oo::BaseId cellId, bool async) {
  auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};

  const auto cellRec{cellRes.get(cellId)};
  if (!cellRec) return;

  CellIndex pos{cellRec->grid->data.x, cellRec->grid->data.y};

  // Check if terrain is already loaded first, if so do nothing.
  if (auto *terrain{mTerrainGroup.getTerrain(2 * qvm::X(pos), 2 * qvm::Y(pos))};
      terrain && terrain->isLoaded()) {
    return;
  }

  if (async) {
    spdlog::get(oo::LOG)->warn("Asynchronous loading of terrain is not "
                               "currently supported, falling back to "
                               "synchronous loading");
    async = false;
  }

  // Begin loading the terrain itself, possibly in the background.
  loadTerrain(pos, async);

  constexpr auto vpc{oo::verticesPerCell<uint32_t>};
  constexpr auto vpq{oo::verticesPerQuad<uint32_t>};

  // Normal data can be generated implicitly by the terrain but instead of
  // being passed as vertex data the normals are saved in a texture. We have
  // explicit normal data in the LAND record so will just generate it ourselves.
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  std::array<uint8_t, vpc * vpc * 3u> normalMapData{};
  Ogre::PixelBox normalMapBox(vpc, vpc, 1, Ogre::PixelFormat::PF_BYTE_RGB,
                              normalMapData.data());

  auto &landRes{oo::getResolver<record::LAND>(mResolvers)};
  record::LAND &landRec{*landRes.get(*cellRes.getLandId(cellId))};

  if (landRec.normals) {
    for (std::size_t y = 0; y < vpc; ++y) {
      for (std::size_t x = 0; x < vpc; ++x) {
        auto[nx, ny, nz]{landRec.normals->data[y * vpc + x]};
        auto n{oo::fromBSCoordinates(Ogre::Vector3(nx, ny, nz))};
        n.normalise();
        normalMapBox.setColourAt(Ogre::ColourValue{n.x, n.y, n.z}, x, y, 0);
      }
    }
  } else {
    // No normal data, use vertical normals
    for (std::size_t y = 0; y < vpc; ++y) {
      for (std::size_t x = 0; x < vpc; ++x) {
        normalMapBox.setColourAt(Ogre::ColourValue{0.0f, 1.0f, 0.0f}, x, y, 0);
      }
    }
  }

  // Vertex colours are also stored in a texture instead of being passed as
  // vertex data.
  std::array<uint8_t, vpc * vpc * 3u> vertexColorData{};
  Ogre::PixelBox vertexColorBox(vpc, vpc, 1, Ogre::PixelFormat::PF_BYTE_RGB,
                                vertexColorData.data());

  if (landRec.colors) {
    for (std::size_t y = 0; y < vpc; ++y) {
      for (std::size_t x = 0; x < vpc; ++x) {
        auto[r, g, b]{landRec.colors->data[y * vpc + x]};
        Ogre::ColourValue col(r / 255.0f, g / 255.0f, b / 255.0f);
        vertexColorBox.setColourAt(col, x, y, 0);
      }
    }
  } else {
    // No vertex colours, use white so textures actually shows up.
    vertexColorData.fill(255u);
  }

  // Build the base texture layer and blend layers.
  auto layerMaps{makeDefaultLayerMaps(landRec)};
  auto layerOrders{makeDefaultLayerOrders(landRec)};

  applyBaseLayers(layerMaps, landRec);
  applyBaseLayers(layerOrders, landRec);

  applyFineLayers(layerMaps, landRec);
  applyFineLayers(layerOrders, landRec);

  // Note that if async then we are not waiting on these pointers being non-null
  // but instead are waiting on their isLoaded().
  std::array<Ogre::Terrain *, 4u> terrain{
      mTerrainGroup.getTerrain(2 * qvm::X(pos) + 0, 2 * qvm::Y(pos) + 0),
      mTerrainGroup.getTerrain(2 * qvm::X(pos) + 1, 2 * qvm::Y(pos) + 0),
      mTerrainGroup.getTerrain(2 * qvm::X(pos) + 0, 2 * qvm::Y(pos) + 1),
      mTerrainGroup.getTerrain(2 * qvm::X(pos) + 1, 2 * qvm::Y(pos) + 1)
  };
  if (std::any_of(terrain.begin(), terrain.end(), std::logical_not<>{})) {
    spdlog::error("Terrain is nullptr at ({}, {})", qvm::X(pos), qvm::Y(pos));
    throw std::runtime_error("Terrain is nullptr");
  }

  auto blitBoxes = [&](const std::string &matName, const Ogre::Box &box) {
    auto np{texMgr.getByName(matName + "normal", oo::RESOURCE_GROUP)};
    np->getBuffer()->blitFromMemory(normalMapBox.getSubVolume(box, true));

    auto vcp{texMgr.getByName(matName + "vertexcolor", oo::RESOURCE_GROUP)};
    vcp->getBuffer()->blitFromMemory(vertexColorBox.getSubVolume(box, true));
  };

  auto blitLayerMaps = [&](std::size_t i) {
    for (uint8_t j = 1; j < layerOrders[i].size(); ++j) {
      const auto id{layerOrders[i][j]};
      const auto &srcMap{layerMaps[i][id]};
      auto *dstMap{terrain[i]->getLayerBlendMap(j)};
      for (std::size_t y = 0; y < vpq; ++y) {
        for (std::size_t x = 0; x < vpq; ++x) {
          const float opacity = srcMap[vpq * y + x] / 255.0f;
          std::size_t s{}, t{};
          dstMap->convertUVToImageSpace(x / (oo::verticesPerQuad<float> - 1.0f),
                                        y / (oo::verticesPerQuad<float> - 1.0f),
                                        &s, &t);
          dstMap->setBlendValue(s, t, opacity);
        }
      }
      dstMap->update();
    }
  };

  // TODO: Properly implement asynchronous terrain loading.
  //       This doesn't actually work because WorkQueue responses are not
  //       processed until the end of the frame, but this function is blocking.
  uint8_t terrainDone{0b0000};
  do {
    if (!(terrainDone & 0b0001) && terrain[0]->isLoaded()) {
      const Ogre::Box box(0u, 0u, vpq, vpq);
      blitBoxes(terrain[0]->getMaterialName(), box);
      blitLayerMaps(0);
      terrainDone |= 0b0001;
    }

    if (!(terrainDone & 0b0010) && terrain[1]->isLoaded()) {
      const Ogre::Box box(vpq - 1u, 0u, vpc, vpq);
      blitBoxes(terrain[1]->getMaterialName(), box);
      blitLayerMaps(1);
      terrainDone |= 0b0010;
    }

    if (!(terrainDone & 0b0100) && terrain[2]->isLoaded()) {
      const Ogre::Box box(0u, vpq - 1u, vpq, vpc);
      blitBoxes(terrain[2]->getMaterialName(), box);
      blitLayerMaps(2);
      terrainDone |= 0b0100;
    }

    if (!(terrainDone & 0b1000) && terrain[3]->isLoaded()) {
      const Ogre::Box box(vpq - 1u, vpq - 1u, vpc, vpc);
      blitBoxes(terrain[3]->getMaterialName(), box);
      blitLayerMaps(3);
      terrainDone |= 0b1000;
    }
  } while (terrainDone != 0b1111);
}

void oo::World::loadTerrain(oo::ExteriorCell &cell) {
  auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};
  loadTerrainOnly(cell.getBaseId(), false);

  const auto cellRec{cellRes.get(cell.getBaseId())};
  if (!cellRec) return;

  CellIndex pos{cellRec->grid->data.x, cellRec->grid->data.y};
  std::array<Ogre::Terrain *, 4u> terrain{
      mTerrainGroup.getTerrain(2 * qvm::X(pos) + 0, 2 * qvm::Y(pos) + 0),
      mTerrainGroup.getTerrain(2 * qvm::X(pos) + 1, 2 * qvm::Y(pos) + 0),
      mTerrainGroup.getTerrain(2 * qvm::X(pos) + 0, 2 * qvm::Y(pos) + 1),
      mTerrainGroup.getTerrain(2 * qvm::X(pos) + 1, 2 * qvm::Y(pos) + 1)
  };

  cell.setTerrain(terrain);
  getPhysicsWorld()->addCollisionObject(cell.getCollisionObject());
}

void oo::World::unloadTerrain(oo::ExteriorCell &cell) {
  getPhysicsWorld()->removeCollisionObject(cell.getCollisionObject());
  auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};

  const auto cellRec{cellRes.get(cell.getBaseId())};
  if (!cellRec) return;

  CellIndex pos{cellRec->grid->data.x, cellRec->grid->data.y};
  unloadTerrain(pos);
}

void oo::World::unloadTerrain(oo::BaseId cellId) {
  auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};

  const auto cellRec{cellRes.get(cellId)};
  if (!cellRec) return;

  CellIndex pos{cellRec->grid->data.x, cellRec->grid->data.y};
  unloadTerrain(pos);
}

void oo::World::unloadTerrain(CellIndex index) {
  auto x{qvm::X(index)};
  auto y{qvm::Y(index)};
  mTerrainGroup.unloadTerrain(2 * x + 0, 2 * y + 0);
  mTerrainGroup.unloadTerrain(2 * x + 1, 2 * y + 0);
  mTerrainGroup.unloadTerrain(2 * x + 0, 2 * y + 1);
  mTerrainGroup.unloadTerrain(2 * x + 1, 2 * y + 1);
}

oo::ReifyRecordTrait<record::WRLD>::type
oo::reifyRecord(const record::WRLD &refRec,
                ReifyRecordTrait<record::WRLD>::resolvers resolvers) {
  const auto &wrldRes{oo::getResolver<record::WRLD>(resolvers)};

  const oo::BaseId baseId{refRec.mFormId};
  std::string name{refRec.name ? refRec.name->data : ""};

  auto world{std::make_shared<oo::World>(baseId, name, resolvers)};

  return world;
}
