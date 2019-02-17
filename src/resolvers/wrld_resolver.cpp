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
                    33u, 4096.0f * oo::metersPerUnit<Ogre::Real>),
      mResolvers(std::move(resolvers)) {
  auto &importData{mTerrainGroup.getDefaultImportSettings()};
  importData.constantHeight = 0.0f;
  importData.inputFloat = nullptr;
  importData.deleteInputData = true;
  importData.inputImage = nullptr;
  importData.terrainSize = 32 + 1;
  importData.terrainAlign = Ogre::Terrain::Alignment::ALIGN_X_Z;
  importData.worldSize = 4096.0f * oo::metersPerUnit<float>;
  importData.maxBatchSize = 32 + 1;
  importData.minBatchSize = 16 + 1;

  // Shift origin because cell coordinates give SW corner position but Ogre
  // works with the centre.
  mTerrainGroup.setOrigin(oo::fromBSCoordinates(Ogre::Vector3{2048, 2048, 0}));
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
    auto importData{mTerrainGroup.getDefaultImportSettings()};
    importData.inputFloat = OGRE_ALLOC_T(float, 33u * 33u,
                                         Ogre::MEMCATEGORY_GEOMETRY);

    // The height data is given as offsets. Moving to the right increases the
    // offset by the height value, moving to a new row resets it to the height
    // of the first value on the row before.
    const float scale{record::raw::VHGT::MULTIPLIER * oo::metersPerUnit<float>};
    float offset{heightRec.offset * scale};
    for (std::size_t j = 0; j < 33u; ++j) {
      offset += heightRec.heights[j * 33u] * scale;
      importData.inputFloat[j * 33u] = offset;

      float h{offset};
      for (std::size_t i = 1; i < 33u; ++i) {
        const float dh = heightRec.heights[j * 33u + i] * scale;
        h += dh;
        importData.inputFloat[j * 33u + i] = h;
      }
    }

    auto &terrainOpts{Ogre::TerrainGlobalOptions::getSingleton()};
    const auto matGen{terrainOpts.getDefaultMaterialGenerator()};
    importData.layerDeclaration = matGen->getLayerDeclaration();

    auto &layer0{importData.layerList.emplace_back()};
    layer0.textureNames
        .emplace_back("textures/landscape/greatforestrockmoss02.dds");
    layer0.textureNames
        .emplace_back("textures/landscape/greatforestrockmoss02_n.dds");
    layer0.worldSize = 1.0f;

    auto &layer1{importData.layerList.emplace_back()};
    layer1.textureNames
        .emplace_back("textures/landscape/terrainhdgrass01su.dds");
    layer1.textureNames
        .emplace_back("textures/landscape/terrainhdgrass01su_n.dds");
    layer1.worldSize = 1.0f;

    mTerrainGroup.defineTerrain(qvm::X(p), qvm::Y(p), &importData);
  }
}

void oo::World::makePhysicsWorld() {
  const auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};
  const auto &bulletConf{cellRes.getBulletConfiguration()};
  mPhysicsWorld = bulletConf.makeDynamicsWorld();
}

void oo::World::makeAtmosphere() {
  // TODO: Use climate and weather data for sun and sky
  mScnMgr->setAmbientLight(Ogre::ColourValue{94.0f, 113.0f, 151.0f} / 255.0f);
  auto *sunNode{mScnMgr->createSceneNode("__sunNode")};
  auto *sunLight{mScnMgr->createLight("__sunLight")};
  sunNode->attachObject(sunLight);

  sunNode
      ->setDirection(0.0f, -1.0f, 0.0f, Ogre::Node::TransformSpace::TS_WORLD);
  sunLight
      ->setDiffuseColour(Ogre::ColourValue{255.0f, 241.0f, 223.0f} / 255.0f);
  sunLight->setType(Ogre::Light::LightTypes::LT_DIRECTIONAL);
}

oo::BaseId oo::World::getCell(CellIndex index) const {
  return mCells[qvm::X(index)][qvm::Y(index)];
}

void oo::World::loadTerrain(CellIndex index) {
  // TODO: Defer the loading of the collision mesh so this can be async
  mTerrainGroup.loadTerrain(qvm::X(index), qvm::Y(index), true);
}

void oo::World::loadTerrain(oo::ExteriorCell &cell) {
  auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};

  const auto cellRec{cellRes.get(cell.getBaseId())};
  if (!cellRec) return;

  CellIndex pos{cellRec->grid->data.x, cellRec->grid->data.y};
  loadTerrain(pos);
  Ogre::Terrain *terrain{mTerrainGroup.getTerrain(qvm::X(pos), qvm::Y(pos))};
  if (!terrain) return;

  // Normal data can be generated implicitly by the terrain but instead of
  // being passed as vertex data the normals are saved in a texture. This
  // texture is in the wrong resource group for us, and since we have explicit
  // normal information in the LAND record we will generate our own texture.
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  auto normalMapPtr{texMgr.getByName(terrain->getMaterialName() + "normal",
                                     oo::RESOURCE_GROUP)};
  std::array<uint8_t, 33u * 33u * 3u> normalMapData{};
  Ogre::PixelBox normalMapBox(33u, 33u, 1, Ogre::PixelFormat::PF_BYTE_RGB,
                              normalMapData.data());

  auto &landRes{oo::getResolver<record::LAND>(mResolvers)};
  record::LAND &landRec{*landRes.get(*cellRes.getLandId(cell.getBaseId()))};

  if (landRec.normals) {
    for (std::size_t y = 0; y < 33u; ++y) {
      for (std::size_t x = 0; x < 33u; ++x) {
        auto[nx, ny, nz]{landRec.normals->data[y * 33u + x]};
        auto n{oo::fromBSCoordinates(Ogre::Vector3(nx, ny, nz))};
        n.normalise();
        normalMapBox.setColourAt(Ogre::ColourValue{n.x, n.y, n.z},
                                 x, 32u - y, 0);
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

  normalMapPtr->getBuffer()->blitFromMemory(normalMapBox);

  // Vertex colours are also set in a texture because the terrain does not
  // provide them.
  auto vertexColorPtr{texMgr.getByName(
      terrain->getMaterialName() + "vertexcolor", oo::RESOURCE_GROUP)};
  std::array<uint8_t, 33u * 33u * 3u> vertexColorData{};
  Ogre::PixelBox vertexColorBox(33u, 33u, 1, Ogre::PixelFormat::PF_BYTE_RGB,
                                vertexColorData.data());

  if (landRec.colors) {
    for (std::size_t y = 0; y < 33u; ++y) {
      for (std::size_t x = 0; x < 33u; ++x) {
        auto[r, g, b]{landRec.colors->data[y * 33u + x]};
        vertexColorBox.setColourAt(Ogre::ColourValue(r, g, b) / 255.0f,
                                   x, 32u - y, 0);
      }
    }
  } else {
    // No vertex colours, use white so textures actually shows up.
    vertexColorData.fill(255u);
  }

  vertexColorPtr->getBuffer()->blitFromMemory(vertexColorBox);

  cell.setTerrain(terrain);
  getPhysicsWorld()->addCollisionObject(cell.getCollisionObject());
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
