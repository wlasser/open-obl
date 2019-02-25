#include "esp.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include "settings.hpp"
#include <OgrePixelFormat.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

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

  auto sunrise{unsigned(oo::chrono::Sunrise)};
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

  auto daytime{unsigned(oo::chrono::Daytime)};
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

  auto sunset{unsigned(oo::chrono::Sunset)};
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

  auto nighttime{unsigned(oo::chrono::Nighttime)};
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
}

void oo::Weather::setSkyDome(Ogre::SceneManager *scnMgr) {
  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  if (!mSkyDomeMaterial) {
    auto matPtr{matMgr.getByName("__skyMaterial", oo::SHADER_GROUP)};
    mSkyDomeMaterial = matPtr->clone("__skyMaterial" + getBaseId().string(),
                                     true, oo::RESOURCE_GROUP);
    if (mLowerCloudsTex && mUpperCloudsTex) {
      Ogre::AliasTextureNamePairList layers{
          {"lowerLayer", mLowerCloudsTex->getName()},
          {"upperLayer", mUpperCloudsTex->getName()}
      };
      mSkyDomeMaterial->applyTextureAliases(layers, true);
    }
  }

  scnMgr->setSkyDome(true, mSkyDomeMaterial->getName(), 10, 8, 4000, true,
                     Ogre::Quaternion::IDENTITY, 16, 16, -1,
                     oo::RESOURCE_GROUP);
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

oo::World::World(oo::BaseId baseId, std::string name, Resolvers resolvers)
    : mBaseId(baseId), mName(std::move(name)),
      mScnMgr(Ogre::Root::getSingleton().createSceneManager()),
      mTerrainGroup(mScnMgr, Ogre::Terrain::Alignment::ALIGN_X_Z,
                    17u, 2048.0f * oo::metersPerUnit<Ogre::Real>),
      mResolvers(std::move(resolvers)) {
  auto &importData{mTerrainGroup.getDefaultImportSettings()};
  importData.constantHeight = 0.0f;
  importData.inputFloat = nullptr;
  importData.deleteInputData = true;
  importData.inputImage = nullptr;
  importData.terrainSize = 16u + 1;
  importData.terrainAlign = Ogre::Terrain::Alignment::ALIGN_X_Z;
  importData.worldSize = 2048.0f * oo::metersPerUnit<float>;
  importData.maxBatchSize = 16u + 1;
  importData.minBatchSize = 8u + 1;

  mTerrainGroup.setResourceGroup(oo::RESOURCE_GROUP);

  // Shift origin because cell coordinates give SW corner position but Ogre
  // works with the centre.
  mTerrainGroup.setOrigin(oo::fromBSCoordinates(Ogre::Vector3{1024, 1024, 0}));
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
    std::array<Ogre::Terrain::ImportData, 4u> importData;
    importData.fill(mTerrainGroup.getDefaultImportSettings());

    setTerrainHeights(heightRec, importData);

    auto &terrainOpts{Ogre::TerrainGlobalOptions::getSingleton()};
    const auto matGen{terrainOpts.getDefaultMaterialGenerator()};
    for (auto &data : importData) {
      data.layerDeclaration = matGen->getLayerDeclaration();
    }

    auto layerMaps{makeDefaultLayerMaps(*landOpt)};
    auto layerOrders{makeDefaultLayerOrders(*landOpt)};
    applyFineTextureLayers(layerOrders, *landOpt);

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

  if (clmt.sunFilename) {
    mHasSun = true;
    oo::Path sunFilename{clmt.sunFilename->data};
    auto *sunNode{mScnMgr->createSceneNode("__sunNode")};
    auto *sunLight{mScnMgr->createLight("__sunLight")};
    sunNode->attachObject(sunLight);

    sunNode->setDirection(0.0f, -1.0f, 0.0f,
                          Ogre::Node::TransformSpace::TS_WORLD);
    sunLight->setType(Ogre::Light::LightTypes::LT_DIRECTIONAL);
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

  auto skyMaterial{weather.getMaterial()};
  auto skyPass = [&skyMaterial]() -> Ogre::Pass * {
    return skyMaterial ? skyMaterial->getTechnique(0)->getPass(0) : nullptr;
  }();
  auto skyParams = [&skyPass]() -> Ogre::GpuProgramParametersSharedPtr {
    return skyPass ? skyPass->getFragmentProgramParameters()
                   : Ogre::GpuProgramParametersSharedPtr();
  }();

  if (mSunriseBegin <= time && time <= mSunriseEnd) {
    // Sunrise
    const auto sunriseMid{(mSunriseEnd + mSunriseBegin) / 2};
    const auto dt{static_cast<float>((sunriseMid - mSunriseBegin).count())};

    if (time <= sunriseMid) {
      const auto c0{weather.getAmbientColor(oo::chrono::Nighttime)};
      const auto s0{weather.getSunlightColor(oo::chrono::Nighttime)};
      const auto ls0{weather.getLowerSkyColor(oo::chrono::Nighttime)};
      const auto us0{weather.getUpperSkyColor(oo::chrono::Nighttime)};
      const auto lc0{weather.getLowerCloudColor(oo::chrono::Nighttime)};
      const auto uc0{weather.getUpperCloudColor(oo::chrono::Nighttime)};
      const auto h0{weather.getHorizonColor(oo::chrono::Nighttime)};

      const auto c1{weather.getAmbientColor(oo::chrono::Sunrise)};
      const auto s1{weather.getSunlightColor(oo::chrono::Sunrise)};
      const auto ls1{weather.getLowerSkyColor(oo::chrono::Sunrise)};
      const auto us1{weather.getUpperSkyColor(oo::chrono::Sunrise)};
      const auto lc1{weather.getLowerCloudColor(oo::chrono::Sunrise)};
      const auto uc1{weather.getUpperCloudColor(oo::chrono::Sunrise)};
      const auto h1{weather.getHorizonColor(oo::chrono::Sunrise)};

      const float t{(time - mSunriseBegin).count() / dt};
      const auto c{(1 - t) * c0 + t * c1};
      const auto s{(1 - t) * s0 + t * s1};
      const auto ls{(1 - t) * ls0 + t * ls1};
      const auto us{(1 - t) * us0 + t * us1};
      const auto lc{(1 - t) * lc0 + t * lc1};
      const auto uc{(1 - t) * uc0 + t * uc1};
      const auto h{(1 - t) * h0 + t * h1};

      mScnMgr->setAmbientLight(c);
      if (light) light->setDiffuseColour(s);
      if (skyParams) {
        skyParams->setNamedConstant("lowerSkyColor", ls);
        skyParams->setNamedConstant("upperSkyColor", us);
        skyParams->setNamedConstant("lowerCloudColor", lc);
        skyParams->setNamedConstant("upperCloudColor", uc);
        skyParams->setNamedConstant("horizonColor", h);
      }
    } else {
      const auto c0{weather.getAmbientColor(oo::chrono::Sunrise)};
      const auto s0{weather.getSunlightColor(oo::chrono::Sunrise)};
      const auto ls0{weather.getLowerSkyColor(oo::chrono::Sunrise)};
      const auto us0{weather.getUpperSkyColor(oo::chrono::Sunrise)};
      const auto lc0{weather.getLowerCloudColor(oo::chrono::Sunrise)};
      const auto uc0{weather.getUpperCloudColor(oo::chrono::Sunrise)};
      const auto h0{weather.getHorizonColor(oo::chrono::Sunrise)};

      const auto c1{weather.getAmbientColor(oo::chrono::Daytime)};
      const auto s1{weather.getSunlightColor(oo::chrono::Daytime)};
      const auto ls1{weather.getLowerSkyColor(oo::chrono::Daytime)};
      const auto us1{weather.getUpperSkyColor(oo::chrono::Daytime)};
      const auto lc1{weather.getLowerCloudColor(oo::chrono::Daytime)};
      const auto uc1{weather.getUpperCloudColor(oo::chrono::Daytime)};
      const auto h1{weather.getHorizonColor(oo::chrono::Daytime)};

      const float t{(time - sunriseMid).count() / dt};
      const auto c{(1 - t) * c0 + t * c1};
      const auto s{(1 - t) * s0 + t * s1};
      const auto ls{(1 - t) * ls0 + t * ls1};
      const auto us{(1 - t) * us0 + t * us1};
      const auto lc{(1 - t) * lc0 + t * lc1};
      const auto uc{(1 - t) * uc0 + t * uc1};
      const auto h{(1 - t) * h0 + t * h1};

      mScnMgr->setAmbientLight(c);
      if (light) light->setDiffuseColour(s);
      if (skyParams) {
        skyParams->setNamedConstant("lowerSkyColor", ls);
        skyParams->setNamedConstant("upperSkyColor", us);
        skyParams->setNamedConstant("lowerCloudColor", lc);
        skyParams->setNamedConstant("upperCloudColor", uc);
        skyParams->setNamedConstant("horizonColor", h);
      }
    }
  } else if (mSunriseEnd < time && time < mSunsetBegin) {
    // Daytime
    const auto c{weather.getAmbientColor(oo::chrono::Daytime)};
    const auto s{weather.getSunlightColor(oo::chrono::Daytime)};
    const auto ls{weather.getLowerSkyColor(oo::chrono::Daytime)};
    const auto us{weather.getUpperSkyColor(oo::chrono::Daytime)};
    const auto lc{weather.getLowerCloudColor(oo::chrono::Daytime)};
    const auto uc{weather.getUpperCloudColor(oo::chrono::Daytime)};
    const auto h{weather.getHorizonColor(oo::chrono::Daytime)};
    mScnMgr->setAmbientLight(c);
    if (light) light->setDiffuseColour(s);
    if (skyParams) {
      skyParams->setNamedConstant("lowerSkyColor", ls);
      skyParams->setNamedConstant("upperSkyColor", us);
      skyParams->setNamedConstant("lowerCloudColor", lc);
      skyParams->setNamedConstant("upperCloudColor", uc);
      skyParams->setNamedConstant("horizonColor", h);
    }
  } else if (mSunsetBegin <= time && time <= mSunsetEnd) {
    // Sunset
    const auto sunsetMid{(mSunsetEnd + mSunsetBegin) / 2};
    const auto dt{static_cast<float>((sunsetMid - mSunsetBegin).count())};

    if (time <= sunsetMid) {
      const auto c0{weather.getAmbientColor(oo::chrono::Daytime)};
      const auto s0{weather.getSunlightColor(oo::chrono::Daytime)};
      const auto ls0{weather.getLowerSkyColor(oo::chrono::Daytime)};
      const auto us0{weather.getUpperSkyColor(oo::chrono::Daytime)};
      const auto lc0{weather.getLowerCloudColor(oo::chrono::Daytime)};
      const auto uc0{weather.getUpperCloudColor(oo::chrono::Daytime)};
      const auto h0{weather.getHorizonColor(oo::chrono::Daytime)};

      const auto c1{weather.getAmbientColor(oo::chrono::Sunset)};
      const auto s1{weather.getSunlightColor(oo::chrono::Sunset)};
      const auto ls1{weather.getLowerSkyColor(oo::chrono::Sunset)};
      const auto us1{weather.getUpperSkyColor(oo::chrono::Sunset)};
      const auto lc1{weather.getLowerCloudColor(oo::chrono::Sunset)};
      const auto uc1{weather.getUpperCloudColor(oo::chrono::Sunset)};
      const auto h1{weather.getHorizonColor(oo::chrono::Sunset)};

      const float t{(time - mSunsetBegin).count() / dt};
      const auto c{(1 - t) * c0 + t * c1};
      const auto s{(1 - t) * s0 + t * s1};
      const auto ls{(1 - t) * ls0 + t * ls1};
      const auto us{(1 - t) * us0 + t * us1};
      const auto lc{(1 - t) * lc0 + t * lc1};
      const auto uc{(1 - t) * uc0 + t * uc1};
      const auto h{(1 - t) * h0 + t * h1};

      mScnMgr->setAmbientLight(c);
      if (light) light->setDiffuseColour(s);
      if (skyParams) {
        skyParams->setNamedConstant("lowerSkyColor", ls);
        skyParams->setNamedConstant("upperSkyColor", us);
        skyParams->setNamedConstant("lowerCloudColor", lc);
        skyParams->setNamedConstant("upperCloudColor", uc);
        skyParams->setNamedConstant("horizonColor", h);
      }
    } else {
      const auto c0{weather.getAmbientColor(oo::chrono::Sunset)};
      const auto s0{weather.getSunlightColor(oo::chrono::Sunset)};
      const auto ls0{weather.getLowerSkyColor(oo::chrono::Sunset)};
      const auto us0{weather.getUpperSkyColor(oo::chrono::Sunset)};
      const auto lc0{weather.getLowerCloudColor(oo::chrono::Sunset)};
      const auto uc0{weather.getUpperCloudColor(oo::chrono::Sunset)};
      const auto h0{weather.getHorizonColor(oo::chrono::Sunset)};

      const auto c1{weather.getAmbientColor(oo::chrono::Nighttime)};
      const auto s1{weather.getSunlightColor(oo::chrono::Nighttime)};
      const auto ls1{weather.getLowerSkyColor(oo::chrono::Nighttime)};
      const auto us1{weather.getUpperSkyColor(oo::chrono::Nighttime)};
      const auto lc1{weather.getLowerCloudColor(oo::chrono::Nighttime)};
      const auto uc1{weather.getUpperCloudColor(oo::chrono::Nighttime)};
      const auto h1{weather.getHorizonColor(oo::chrono::Nighttime)};

      const float t{(time - sunsetMid).count() / dt};
      const auto c{(1 - t) * c0 + t * c1};
      const auto s{(1 - t) * s0 + t * s1};
      const auto ls{(1 - t) * ls0 + t * ls1};
      const auto us{(1 - t) * us0 + t * us1};
      const auto lc{(1 - t) * lc0 + t * lc1};
      const auto uc{(1 - t) * uc0 + t * uc1};
      const auto h{(1 - t) * h0 + t * h1};

      mScnMgr->setAmbientLight(c);
      if (light) light->setDiffuseColour(s);
      if (skyParams) {
        skyParams->setNamedConstant("lowerSkyColor", ls);
        skyParams->setNamedConstant("upperSkyColor", us);
        skyParams->setNamedConstant("lowerCloudColor", lc);
        skyParams->setNamedConstant("upperCloudColor", uc);
        skyParams->setNamedConstant("horizonColor", h);
      }
    }
  } else {
    // Nighttime
    const auto c{weather.getAmbientColor(oo::chrono::Nighttime)};
    const auto s{weather.getSunlightColor(oo::chrono::Nighttime)};
    const auto ls{weather.getLowerSkyColor(oo::chrono::Nighttime)};
    const auto us{weather.getUpperSkyColor(oo::chrono::Nighttime)};
    const auto lc{weather.getLowerCloudColor(oo::chrono::Nighttime)};
    const auto uc{weather.getUpperCloudColor(oo::chrono::Nighttime)};
    const auto h{weather.getHorizonColor(oo::chrono::Nighttime)};
    mScnMgr->setAmbientLight(c);
    if (light) light->setDiffuseColour(s);
    if (skyParams) {
      skyParams->setNamedConstant("lowerSkyColor", ls);
      skyParams->setNamedConstant("upperSkyColor", us);
      skyParams->setNamedConstant("lowerCloudColor", lc);
      skyParams->setNamedConstant("upperCloudColor", uc);
      skyParams->setNamedConstant("horizonColor", h);
    }
  }
}

void oo::World::setTerrainHeights(const record::raw::VHGT &rec,
                                  std::array<Ogre::Terrain::ImportData,
                                             4u> &importData) const {
  // Allocation method required for Ogre to manage the memory and delete it.
  for (auto &data : importData) {
    data.inputFloat = OGRE_ALLOC_T(float, 17u * 17u,
                                   Ogre::MEMCATEGORY_GEOMETRY);
  }

  // The height data is given as offsets. Moving to the right increases the
  // offset by the height value, moving to a new row resets it to the height
  // of the first value on the row before.
  const float scale{record::raw::VHGT::MULTIPLIER * oo::metersPerUnit<float>};
  float rowStartHeight{rec.offset * scale};

  // Because of the offsets its much easier to treat the entire cell as a whole,
  // then pull out the quadrants afterwards.
  std::array<float, 33u * 33u> tmp{};

  for (std::size_t j = 0; j < 33u; ++j) {
    const std::size_t o{j * 33u};

    rowStartHeight += rec.heights[o] * scale;
    tmp[o] = rowStartHeight;

    float height{rowStartHeight};
    for (std::size_t i = 1; i < 33u; ++i) {
      height += rec.heights[o + i] * scale;
      tmp[o + i] = height;
    }
  }

  for (std::size_t j = 0; j < 17u; ++j) {
    std::memcpy(&importData[0].inputFloat[17u * j], &tmp[33u * j], 17u * 4u);
  }

  for (std::size_t j = 0; j < 17u; ++j) {
    std::memcpy(&importData[1].inputFloat[17u * j], &tmp[33u * j + 16u],
                17u * 4u);
  }

  for (std::size_t j = 0; j < 17u; ++j) {
    std::memcpy(&importData[2].inputFloat[17u * j], &tmp[33u * (j + 16u)],
                17u * 4u);
  }

  for (std::size_t j = 0; j < 17u; ++j) {
    std::memcpy(&importData[3].inputFloat[17u * j], &tmp[33u * (j + 16u) + 16u],
                17u * 4u);
  }
}

void
oo::World::emplaceTexture(Ogre::StringVector &list, std::string texName) const {
  std::string fullName{"textures/landscape/" + std::move(texName)};
  list.emplace_back(fullName);
  fullName.replace(fullName.begin() + fullName.find_last_of('.'),
                   fullName.end(), "_n.dds");
  list.emplace_back(std::move(fullName));
}

std::array<oo::World::LayerMap, 4u>
oo::World::makeDefaultLayerMaps(const record::LAND &rec) const {
  std::array<LayerMap, 4u> layerMaps;

  std::generate(layerMaps.begin(), layerMaps.end(), []() -> LayerMap {
    LayerMap layers{};
    layers[oo::BaseId{0}].fill(255);
    return layers;
  });

  // Find all the quadrant base textures, overwriting the default layer.
  for (const record::BTXT &quadrantTexture : rec.quadrantTexture) {
    const int quadrant{quadrantTexture.data.quadrant};
    oo::BaseId id{quadrantTexture.data.id};
    layerMaps[quadrant].clear();
    layerMaps[quadrant][id].fill(255);
  }

  return layerMaps;
}

std::array<oo::World::LayerOrder, 4u>
oo::World::makeDefaultLayerOrders(const record::LAND &rec) const {
  std::array<LayerOrder, 4u> layerOrders;
  std::generate(layerOrders.begin(), layerOrders.end(), []() -> LayerOrder {
    return std::vector{oo::BaseId{0}};
  });

  // Find all the quadrant base textures, overwriting the default layer.
  for (const record::BTXT &quadrantTexture : rec.quadrantTexture) {
    const int quadrant{quadrantTexture.data.quadrant};
    oo::BaseId id{quadrantTexture.data.id};
    layerOrders[quadrant][0] = id;
  }

  return layerOrders;
}

void oo::World::applyFineTextureLayers(std::array<LayerMap, 4u> &layerMaps,
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

void oo::World::applyFineTextureLayers(std::array<LayerOrder, 4u> &layerOrders,
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
  loadTerrain(pos, async);

  std::array<Ogre::Terrain *, 4u> terrain{
      mTerrainGroup.getTerrain(2 * qvm::X(pos) + 0, 2 * qvm::Y(pos) + 0),
      mTerrainGroup.getTerrain(2 * qvm::X(pos) + 1, 2 * qvm::Y(pos) + 0),
      mTerrainGroup.getTerrain(2 * qvm::X(pos) + 0, 2 * qvm::Y(pos) + 1),
      mTerrainGroup.getTerrain(2 * qvm::X(pos) + 1, 2 * qvm::Y(pos) + 1)
  };
  if (std::any_of(terrain.begin(), terrain.end(), std::logical_not<>{})) return;

  // Normal data can be generated implicitly by the terrain but instead of
  // being passed as vertex data the normals are saved in a texture. This
  // texture is in the wrong resource group for us, and since we have explicit
  // normal information in the LAND record we will generate our own texture.
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  std::array<uint8_t, 33u * 33u * 3u> normalMapData{};
  Ogre::PixelBox normalMapBox(33u, 33u, 1, Ogre::PixelFormat::PF_BYTE_RGB,
                              normalMapData.data());

  auto &landRes{oo::getResolver<record::LAND>(mResolvers)};
  record::LAND &landRec{*landRes.get(*cellRes.getLandId(cellId))};

  if (landRec.normals) {
    for (std::size_t y = 0; y < 33u; ++y) {
      for (std::size_t x = 0; x < 33u; ++x) {
        auto[nx, ny, nz]{landRec.normals->data[y * 33u + x]};
        auto n{oo::fromBSCoordinates(Ogre::Vector3(nx, ny, nz))};
        n.normalise();
        normalMapBox.setColourAt(Ogre::ColourValue{n.x, n.y, n.z}, x, y, 0);
      }
    }
  } else {
    // No normal data, use vertical normals
    for (std::size_t y = 0; y < 33; ++y) {
      for (std::size_t x = 0; x < 33; ++x) {
        normalMapBox.setColourAt(Ogre::ColourValue{0.0f, 1.0f, 0.0f}, x, y, 0);
      }
    }
  }

  {
    auto ptr{texMgr.getByName(terrain[0]->getMaterialName() + "normal",
                              oo::RESOURCE_GROUP)};
    ptr->getBuffer()->blitFromMemory(normalMapBox.getSubVolume(
        Ogre::Box(0u, 0u, 17u, 17u), /*resetOrigin=*/true));
  }
  {
    auto ptr{texMgr.getByName(terrain[1]->getMaterialName() + "normal",
                              oo::RESOURCE_GROUP)};
    ptr->getBuffer()->blitFromMemory(normalMapBox.getSubVolume(
        Ogre::Box(16u, 0u, 33u, 17u), /*resetOrigin=*/true));
  }
  {
    auto ptr{texMgr.getByName(terrain[2]->getMaterialName() + "normal",
                              oo::RESOURCE_GROUP)};
    ptr->getBuffer()->blitFromMemory(normalMapBox.getSubVolume(
        Ogre::Box(0u, 16u, 17u, 33u), /*resetOrigin=*/true));
  }
  {
    auto ptr{texMgr.getByName(terrain[3]->getMaterialName() + "normal",
                              oo::RESOURCE_GROUP)};
    ptr->getBuffer()->blitFromMemory(normalMapBox.getSubVolume(
        Ogre::Box(16u, 16u, 33u, 33u), /*resetOrigin=*/true));
  }

  std::array<uint8_t, 33u * 33u * 3u> vertexColorData{};
  Ogre::PixelBox vertexColorBox(33u, 33u, 1, Ogre::PixelFormat::PF_BYTE_RGB,
                                vertexColorData.data());

  if (landRec.colors) {
    for (std::size_t y = 0; y < 33u; ++y) {
      for (std::size_t x = 0; x < 33u; ++x) {
        auto[r, g, b]{landRec.colors->data[y * 33u + x]};
        Ogre::ColourValue col(r / 255.0f, g / 255.0f, b / 255.0f);
        vertexColorBox.setColourAt(col, x, y, 0);
      }
    }
  } else {
    // No vertex colours, use white so textures actually shows up.
    vertexColorData.fill(255u);
  }

  {
    auto ptr{texMgr.getByName(terrain[0]->getMaterialName() + "vertexcolor",
                              oo::RESOURCE_GROUP)};
    ptr->getBuffer()->blitFromMemory(vertexColorBox.getSubVolume(
        Ogre::Box(0u, 0u, 17u, 17u), /*resetOrigin=*/true));
  }
  {
    auto ptr{texMgr.getByName(terrain[1]->getMaterialName() + "vertexcolor",
                              oo::RESOURCE_GROUP)};
    ptr->getBuffer()->blitFromMemory(vertexColorBox.getSubVolume(
        Ogre::Box(16u, 0u, 33u, 17u), /*resetOrigin=*/true));
  }
  {
    auto ptr{texMgr.getByName(terrain[2]->getMaterialName() + "vertexcolor",
                              oo::RESOURCE_GROUP)};
    ptr->getBuffer()->blitFromMemory(vertexColorBox.getSubVolume(
        Ogre::Box(0u, 16u, 17u, 33u), /*resetOrigin=*/true));
  }
  {
    auto ptr{texMgr.getByName(terrain[3]->getMaterialName() + "vertexcolor",
                              oo::RESOURCE_GROUP)};
    ptr->getBuffer()->blitFromMemory(vertexColorBox.getSubVolume(
        Ogre::Box(16u, 16u, 33u, 33u), /*resetOrigin=*/true));
  }

  auto layerMaps{makeDefaultLayerMaps(landRec)};
  auto layerOrders{makeDefaultLayerOrders(landRec)};

  applyFineTextureLayers(layerMaps, landRec);
  applyFineTextureLayers(layerOrders, landRec);

  for (std::size_t i = 0; i < 4; ++i) {
    for (uint8_t j = 1; j < layerOrders[i].size(); ++j) {
      const auto id{layerOrders[i][j]};
      const auto &srcMap{layerMaps[i][id]};
      auto *dstMap{terrain[i]->getLayerBlendMap(j)};
      for (std::size_t y = 0; y < 17; ++y) {
        for (std::size_t x = 0; x < 17; ++x) {
          const float opacity = srcMap[17 * y + x] / 255.0f;
          std::size_t s{}, t{};
          dstMap->convertUVToImageSpace(x / 16.0f, y / 16.0f, &s, &t);
          dstMap->setBlendValue(s, t, opacity);
        }
      }
      dstMap->update();
    }
  }
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
