#include "esp.hpp"
#include "job/job.hpp"
#include "mesh/mesh_manager.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include "settings.hpp"
#include <OgreHardwarePixelBuffer.h>
#include <OgreInstanceBatch.h>
#include <OgreInstanceManager.h>
#include <OgreInstancedEntity.h>
#include <OgreMaterialManager.h>
#include <OgreMeshManager.h>
#include <OgreMovableObject.h>
#include <OgrePixelFormat.h>
#include <OgreRoot.h>
#include <OgreTextureManager.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <Terrain/OgreTerrainGroup.h>

namespace oo {

//===----------------------------------------------------------------------===//
// WRLD resolver definitions
//===----------------------------------------------------------------------===//

Resolver<record::WRLD, oo::BaseId>::Resolver(Resolver &&other) noexcept {
  std::scoped_lock lock{other.mMtx};
  mRecords = std::move(other.mRecords);
}

WrldResolver &
Resolver<record::WRLD, oo::BaseId>::operator=(Resolver &&other) noexcept {
  if (this != &other) {
    std::scoped_lock lock{mMtx, other.mMtx};
    using std::swap;
    swap(mRecords, other.mRecords);
  }

  return *this;
}

std::pair<WrldResolver::RecordIterator, bool>
WrldResolver::insertOrAppend(oo::BaseId baseId,
                             const record::WRLD &rec,
                             oo::EspAccessor accessor) {
  std::scoped_lock lock{mMtx};
  spdlog::get(oo::LOG)->info("Inserting WRLD {}, {}", baseId,
                             rec.editorId.data);

  RecordEntry entry{std::make_pair(rec, tl::nullopt)};
  Metadata meta{{accessor}, {}, oo::CellGrid{}, {}};
  auto[it, inserted]{mRecords.try_emplace(baseId, entry, meta)};
  if (inserted) return {it, inserted};

  auto &wrappedEntry{it->second};
  wrappedEntry.second.mAccessors.push_back(accessor);
  wrappedEntry.first = std::make_pair(rec, tl::nullopt);

  return {it, inserted};
}

tl::optional<const record::WRLD &> WrldResolver::get(oo::BaseId baseId) const {
  std::scoped_lock lock{mMtx};
  const auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  const auto &entry{it->second.first};

  return entry.second ? *entry.second : entry.first;
}

/// \overload get(oo::BaseId)
tl::optional<record::WRLD &> WrldResolver::get(oo::BaseId baseId) {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  auto &entry{it->second.first};

  if (!entry.second) entry.second.emplace(entry.first);
  return *entry.second;
}

/// Check if there is a world with the baseId.
bool WrldResolver::contains(oo::BaseId baseId) const {
  std::scoped_lock lock{mMtx};
  return mRecords.find(baseId) != mRecords.end();
}

void WrldResolver::load(oo::BaseId baseId, BaseResolverContext baseCtx) {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return;
  auto &meta{it->second.second};
  meta.mCells.clear();
  meta.mPersistentReferences.clear();

  // Get the actual WRLD record
  auto &entry{it->second.first};
  const record::WRLD &rec{entry.second ? *entry.second : entry.first};

  // Worldspace bounds, in units.
  const auto[x0, y0]{rec.bottomLeft.data};
  const auto[x1, y1]{rec.topRight.data};

  // Worldspace bounds, in cells.
  const auto p0{oo::getCellIndex(x0, y0)};
  const auto p1{oo::getCellIndex(x1, y1)};

  meta.mCellGrid.resize(boost::extents[qvm::X(p1 - p0)][qvm::Y(p1 - p0)]);
  meta.mCellGrid.reindex(std::array{qvm::X(p0), qvm::Y(p0)});

  WrldVisitor visitor(meta, baseCtx);
  // Taking accessors by value so subsequent reads will work
  for (auto accessor : meta.mAccessors) {
    oo::readWrldChildren(accessor, oo::SkipGroupVisitorTag, visitor);
  }
}

tl::optional<oo::BaseId>
WrldResolver::getCell(oo::BaseId baseId, oo::CellIndex index) const {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  const auto &grid{it->second.second.mCellGrid};
  const auto x{qvm::X(index)}, y{qvm::Y(index)};

  // Multiarray has no builtin way to do bounds checking :(
  // By default it asserts on out-of-bounds, not throws, so we can't even 'try'.
  if (x < grid.index_bases()[0]
      || x > grid.index_bases()[0] + static_cast<int>(grid.shape()[0])
      || y < grid.index_bases()[1]
      || y > grid.index_bases()[1] + static_cast<int>(grid.shape()[1])) {
    return tl::nullopt;
  }

  return grid[x][y];
}

oo::CellGridView
WrldResolver::getNeighbourhood(oo::BaseId wrldId,
                               CellIndex cell,
                               int diameter) const {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(wrldId)};
  // We are allowed to UB if the set of cells is empty, so this is fine.
  const auto &grid{it->second.second.mCellGrid};

  const int x{qvm::X(cell)}, y{qvm::Y(cell)};
  // Adding 1 maps intervals (a, b] -> [a, b)

  // For integer i and real r,
  // floor(i - r) = i - ceil(r)
  const int x0{1 + x - diameter / 2 - (diameter % 2 == 0 ? 0 : 1)};
  const int y0{1 + y - diameter / 2 - (diameter % 2 == 0 ? 0 : 1)};
  // floor(i + r) = i + floor(r)
  const int x1{1 + x + diameter / 2};
  const int y1{1 + y + diameter / 2};

  // Keep within bounds.
  const int X0{std::max<int>(x0, grid.index_bases()[0])};
  const int Y0{std::max<int>(y0, grid.index_bases()[1])};
  const int X1{std::min<int>(x1, grid.index_bases()[0] + grid.shape()[0])};
  const int Y1{std::min<int>(y1, grid.index_bases()[1] + grid.shape()[1])};

  using Range = CellGrid::index_range;
  return grid[boost::indices[Range(X0, X1)][Range(Y0, Y1)]];
}

tl::optional<const std::unordered_set<oo::BaseId> &>
WrldResolver::getCells(oo::BaseId baseId) const {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  const auto &cells{it->second.second.mCells};
  if (cells.empty()) return tl::nullopt;
  return cells;
}

std::unordered_set<oo::BaseId> WrldResolver::getWorlds() const {
  std::scoped_lock lock{mMtx};
  std::unordered_set<oo::BaseId> ids{};
  for (const auto &[id, _] : mRecords) ids.emplace(id);
  return ids;
}

template<> void
WrldResolver::WrldVisitor::readRecord<record::CELL>(oo::EspAccessor &accessor) {
  auto &cellRes{oo::getResolver<record::CELL>(mBaseCtx)};
  const auto rec{accessor.readRecord<record::CELL>().value};
  const oo::BaseId baseId{rec.mFormId};
  cellRes.insertOrAppend(baseId, rec, accessor, /*isExterior=*/true);

  mMeta.mCells.emplace(baseId);
  const auto gridOpt{rec.grid};
  if (gridOpt) {
    oo::CellIndex p{gridOpt->data.x, gridOpt->data.y};
    mMeta.mCellGrid[qvm::X(p)][qvm::Y(p)] = baseId;
  }

  if (accessor.peekGroupType() == record::Group::GroupType::CellChildren) {
    accessor.skipGroup();
  }
}

//===----------------------------------------------------------------------===//
// World definitions
//===----------------------------------------------------------------------===//

oo::BaseId World::getBaseId() const {
  return mBaseId;
}

std::string World::getName() const {
  return mName;
}

void World::setName(std::string name) {
  mName = std::move(name);
}

gsl::not_null<Ogre::SceneManager *> World::getSceneManager() const {
  return gsl::make_not_null(mScnMgr.get());
}

gsl::not_null<World::PhysicsWorld *> World::getPhysicsWorld() const {
  return gsl::make_not_null(mPhysicsWorld.get());
}

void World::setDefaultImportData() {
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

World::World(oo::BaseId baseId, std::string name, Resolvers resolvers)
    : mBaseId(baseId), mName(std::move(name)),
      mScnMgr(Ogre::Root::getSingleton().createSceneManager(
          "oo::DeferredSceneManager"),
              [](Ogre::SceneManager *p) {
                auto root{Ogre::Root::getSingletonPtr()};
                if (root) root->destroySceneManager(p);
              }),
      mTerrainGroup(mScnMgr.get(), Ogre::Terrain::Alignment::ALIGN_X_Z,
                    oo::verticesPerQuad<uint16_t>,
                    oo::metersPerUnit<Ogre::Real> * oo::unitsPerQuad<float>),
      mResolvers(std::move(resolvers)),
      mAtmosphere(baseId, gsl::make_not_null(mScnMgr.get()),
                  oo::getResolvers<record::WTHR, record::CLMT, record::WRLD>(
                      mResolvers)) {
  // Shift origin because cell coordinates give SW corner position but Ogre
  // works with the centre.
  mTerrainGroup.setOrigin(oo::fromBSCoordinates(Ogre::Vector3{
      oo::unitsPerQuad<float> / 2.0f,
      oo::unitsPerQuad<float> / 2.0f,
      0.0f
  }));
  mTerrainGroup.setResourceGroup(oo::RESOURCE_GROUP);
  setDefaultImportData();

  auto logger{spdlog::get(oo::LOG)};

  auto &meshMgr{Ogre::MeshManager::getSingleton()};
  if (!meshMgr.resourceExists(WATER_MESH_NAME, oo::RESOURCE_GROUP)) {
    meshMgr.createPlane(WATER_MESH_NAME, oo::RESOURCE_GROUP,
                        Ogre::Plane(Ogre::Vector3::UNIT_Y, /*height*/0.0f),
                        oo::unitsPerCell<float> * oo::metersPerUnit<float>,
                        oo::unitsPerCell<float> * oo::metersPerUnit<float>,
                        1, 1, true, 1u, 1.0f, 1.0f,
                        Ogre::Vector3::UNIT_Z);
  }
  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  const auto waterMatName{WATER_BASE_MATERIAL + getBaseId().string()};
  if (!matMgr.resourceExists(waterMatName, oo::RESOURCE_GROUP)) {
    auto matPtr{matMgr.getByName(WATER_BASE_MATERIAL, oo::SHADER_GROUP)};
    auto newMatPtr{matPtr->clone(waterMatName, /*changeGroup=*/true,
                                 oo::RESOURCE_GROUP)};

    if (const auto watrIdOpt{getWatrId()}) {
      auto &watrRes{oo::getResolver<record::WATR>(mResolvers)};
      if (const auto watrOpt{watrRes.get(*watrIdOpt)}) {
        const auto &texFile{watrOpt->textureFilename};
        oo::Path watrBasePath{texFile && !texFile->data.empty()
                              ? texFile->data : "water/water00.dds"};
        oo::Path watrPath{oo::Path{"textures"} / std::move(watrBasePath)};
        Ogre::AliasTextureNamePairList layers{
            {"diffuse", watrPath.c_str()}
        };
        newMatPtr->applyTextureAliases(layers, true);
      } else {
        logger->warn("WRLD {}: WATR record {} does not exist",
                     mBaseId, *watrIdOpt);
      }
    } else {
      logger->warn("WRLD {}: No NAM2 record in this or any ancestors", mBaseId);
    }

    newMatPtr->load();
  }

  auto *instMgr{mScnMgr->createInstanceManager(
      WATER_MANAGER_BASE_NAME + mBaseId.string(),
      WATER_MESH_NAME, oo::RESOURCE_GROUP,
      Ogre::InstanceManager::InstancingTechnique::HWInstancingBasic,
      /*instancesPerBatch*/32)};
  instMgr->setSetting(Ogre::InstanceManager::BatchSettingId::CAST_SHADOWS,
                      false);

  logger->info("WRLD {}: Making physics world...", baseId);
  makePhysicsWorld();
  boost::this_fiber::yield();

  logger->info("WRLD {}: Making cell grid...", baseId);
  makeCellGrid();
  boost::this_fiber::yield();
}

World::~World() {
  mTerrainGroup.removeAllTerrains();
}

tl::optional<oo::BaseId> World::getLandId(oo::BaseId cellId) {
  return getLandId(cellId, mBaseId);
}

tl::optional<oo::BaseId>
World::getLandId(oo::BaseId cellId, oo::BaseId wrldId) {
  auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};
  auto &wrldRes{oo::getResolver<record::WRLD>(mResolvers)};

  const record::WRLD &wrldRec{*wrldRes.get(wrldId)};

  if (!wrldRec.parentWorldspace) {
    // No parent worldspace, just load the cells' terrain and use that its
    // LAND record.
    cellRes.loadTerrain(cellId, oo::getResolvers<record::LAND>(mResolvers));
    auto landId{cellRes.getLandId(cellId)};
    if (landId) return landId;

    // No LAND record but no parent worldspace to go to.
    spdlog::get(oo::LOG)->warn("Cell {} in World {} has no LAND record and "
                               "the World has no parent worldspace",
                               cellId, wrldId);
    return tl::nullopt;
  } else {
    // Look up the cell with the same index in the parent worldspace and use
    // its LAND record.
    const auto cellOpt{cellRes.get(cellId)};
    if (!cellOpt) {
      spdlog::get(oo::LOG)->warn("Cell {} in World {} not found",
                                 cellId, wrldId);
      return tl::nullopt;
    }

    const auto gridOpt{cellOpt->grid};
    if (!gridOpt) {
      spdlog::get(oo::LOG)->warn("Cell {} in World {} has no XCLC record",
                                 cellId, wrldId);
      return tl::nullopt;
    }
    oo::CellIndex pos{gridOpt->data.x, gridOpt->data.y};

    const oo::BaseId parentWrldId{wrldRec.parentWorldspace->data};
    if (!wrldRes.contains(parentWrldId)) {
      spdlog::get(oo::LOG)->warn("Parent World {} of World {} not found",
                                 parentWrldId, wrldId);
      return tl::nullopt;
    }

    // TODO: Add a builtin function for testing if a WRLD is loaded.
    if (!wrldRes.getCells(parentWrldId)) {
      wrldRes.load(parentWrldId, oo::getResolvers<record::CELL>(mResolvers));
    }

    const auto parentCellIdOpt{wrldRes.getCell(parentWrldId, pos)};
    if (!parentCellIdOpt) {
      spdlog::get(oo::LOG)->warn("Parent of Cell {} not found", cellId);
      return tl::nullopt;
    }
    const oo::BaseId parentCellId{*parentCellIdOpt};

    // Try to load the LAND record of this parent cell, and if that fails keep
    // going up through parent worldspaces until we succeed or run into an error.
    auto landId{getLandId(parentCellId, parentWrldId)};
    if (!landId) {
      spdlog::get(oo::LOG)->warn("Neither Cell {} nor parent Cell {} has a "
                                 "LAND record", cellId, parentCellId);
      return tl::nullopt;
    }

    return landId;
  }
}

tl::optional<oo::BaseId> World::getWatrId() {
  return getWatrId(mBaseId);
}

tl::optional<oo::BaseId>
World::getWatrId(oo::BaseId wrldId) {
  auto &wrldRes{oo::getResolver<record::WRLD>(mResolvers)};

  const record::WRLD &wrldRec{*wrldRes.get(wrldId)};

  if (!wrldRec.parentWorldspace) {
    // TODO: Use tl::optional everywhere.
    return wrldRec.water ? tl::optional{wrldRec.water->data} : tl::nullopt;
  } else {
    return getWatrId(wrldRec.parentWorldspace->data);
  }

  return tl::nullopt;
}

void World::makeCellGrid() {
  auto &wrldRes{oo::getResolver<record::WRLD>(mResolvers)};

  // Number of cells to load before yielding this fiber. We have a *lot* of
  // cells to load, and yielding after every cell is slow.
  constexpr unsigned CELLS_PER_YIELD{64};
  // Increment this every cell load and reset to zero once we yield.
  unsigned cellsAfterYield{0};

  // Need non-const CELL and LAND for loadTerrain, then const LAND.
  auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};
  const auto &landRes{oo::getResolver<record::LAND>(mResolvers)};
  const auto &ltexRes{oo::getResolver<record::LTEX>(mResolvers)};

  for (auto cellId : *wrldRes.getCells(mBaseId)) {
    auto landId{getLandId(cellId)};
    if (!landId) continue;

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

    auto layerOrders{makeDefaultLayerOrders()};
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

    // Note: The success of getLandId() implies that the cell record exists.
    const auto gridOpt{cellRes.get(cellId)->grid};
    if (!gridOpt) {
      spdlog::get(oo::LOG)->warn("Cell {} in World {} has no XCLC record",
                                 cellId, mBaseId);
      continue;
    }

    const auto x{gridOpt->data.x};
    const auto y{gridOpt->data.y};
    mTerrainGroup.defineTerrain(2 * x + 0, 2 * y + 0, &importData[0]);
    mTerrainGroup.defineTerrain(2 * x + 1, 2 * y + 0, &importData[1]);
    mTerrainGroup.defineTerrain(2 * x + 0, 2 * y + 1, &importData[2]);
    mTerrainGroup.defineTerrain(2 * x + 1, 2 * y + 1, &importData[3]);

    if (++cellsAfterYield > CELLS_PER_YIELD) {
      cellsAfterYield = 0u;
      boost::this_fiber::yield();
    }
  }
}

void World::makePhysicsWorld() {
  const auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};
  const auto &bulletConf{cellRes.getBulletConfiguration()};
  mPhysicsWorld = bulletConf.makeDynamicsWorld();
}

void World::updateAtmosphere(const oo::chrono::minutes &time) {
  mAtmosphere.update(time);
}

void World::setTerrainHeights(const record::raw::VHGT &rec,
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
World::emplaceTexture(Ogre::StringVector &list, std::string texName) const {
  std::string fullName{"textures/landscape/" + std::move(texName)};
  list.emplace_back(fullName);
  std::string normalName{oo::makeNormalPath(fullName)};

  auto &texMgr{Ogre::TextureManager::getSingleton()};
  if (texMgr.resourceExists(normalName, oo::RESOURCE_GROUP)) {
    list.emplace_back(std::move(normalName));
  } else {
    list.emplace_back("textures/flat_n.dds");
  }
}

World::LayerMaps World::makeDefaultLayerMaps() const {
  LayerMaps layerMaps;
  std::generate(layerMaps.begin(), layerMaps.end(), []() -> LayerMap {
    LayerMap layers{};
    layers[oo::BaseId{0}].fill(255);
    return layers;
  });

  return layerMaps;
}

World::LayerOrders World::makeDefaultLayerOrders() const {
  LayerOrders layerOrders;
  std::generate(layerOrders.begin(), layerOrders.end(), []() -> LayerOrder {
    return std::vector{oo::BaseId{0}};
  });

  return layerOrders;
}

void
World::applyBaseLayers(LayerMaps &layerMaps, const record::LAND &rec) const {
  // Find all the quadrant base textures, overwriting the default layer.
  for (const record::BTXT &quadrantTexture : rec.quadrantTexture) {
    const int quadrant{quadrantTexture.data.quadrant};
    oo::BaseId id{quadrantTexture.data.id};
    layerMaps[quadrant].clear();
    layerMaps[quadrant][id].fill(255);
  }
}

void World::applyBaseLayers(LayerOrders &layerOrders,
                            const record::LAND &rec) const {
  // Find all the quadrant base textures, overwriting the default layer.
  for (const record::BTXT &quadrantTexture : rec.quadrantTexture) {
    const int quadrant{quadrantTexture.data.quadrant};
    oo::BaseId id{quadrantTexture.data.id};
    layerOrders[quadrant][0] = id;
  }
}

void
World::applyFineLayers(LayerMaps &layerMaps, const record::LAND &rec) const {
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

void World::applyFineLayers(LayerOrders &layerOrders,
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

std::shared_ptr<oo::JobCounter>
World::loadTerrain(CellIndex index, bool async) {
  auto x{qvm::X(index)};
  auto y{qvm::Y(index)};
  if (async) {
    auto jc{std::make_shared<oo::JobCounter>(4)};
    oo::RenderJobManager::runJob([&group = mTerrainGroup, x, y]() {
      spdlog::get(oo::LOG)->info("[{}]: Loading terrain 0",
                                 boost::this_fiber::get_id());
      group.loadTerrain(2 * x + 0, 2 * y + 0, true);
    }, jc.get());
    oo::RenderJobManager::runJob([&group = mTerrainGroup, x, y]() {
      spdlog::get(oo::LOG)->info("[{}]: Loading terrain 1",
                                 boost::this_fiber::get_id());
      group.loadTerrain(2 * x + 1, 2 * y + 0, true);
    }, jc.get());
    oo::RenderJobManager::runJob([&group = mTerrainGroup, x, y]() {
      spdlog::get(oo::LOG)->info("[{}]: Loading terrain 2",
                                 boost::this_fiber::get_id());
      group.loadTerrain(2 * x + 0, 2 * y + 1, true);
    }, jc.get());
    oo::RenderJobManager::runJob([&group = mTerrainGroup, x, y]() {
      spdlog::get(oo::LOG)->info("[{}]: Loading terrain 3",
                                 boost::this_fiber::get_id());
      group.loadTerrain(2 * x + 1, 2 * y + 1, true);
    }, jc.get());

    return jc;
  }

  mTerrainGroup.loadTerrain(2 * x + 0, 2 * y + 0, true);
  mTerrainGroup.loadTerrain(2 * x + 1, 2 * y + 0, true);
  mTerrainGroup.loadTerrain(2 * x + 0, 2 * y + 1, true);
  mTerrainGroup.loadTerrain(2 * x + 1, 2 * y + 1, true);
  return std::shared_ptr<oo::JobCounter>();
}

void World::loadTerrainOnly(oo::BaseId cellId, bool async) {
  auto logger{spdlog::get(oo::LOG)};
  logger->info("[{}]: loadTerrainOnly({})",
               boost::this_fiber::get_id(), cellId);
  auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};

  const auto cellRec{cellRes.get(cellId)};
  if (!cellRec) return;

  CellIndex pos{cellRec->grid->data.x, cellRec->grid->data.y};

  // Check if terrain is already loaded first, if so do nothing.
  if (auto *terrain{mTerrainGroup.getTerrain(2 * qvm::X(pos), 2 * qvm::Y(pos))};
      terrain && terrain->isLoaded()) {
    logger->info("[{}]: Terrain is already loaded",
                 boost::this_fiber::get_id());
    return;
  }

  // Begin loading the terrain itself
  auto terrainCounter{loadTerrain(pos, async)};
  logger->info("[{}]: Started loadTerrain() jobs", boost::this_fiber::get_id());

  constexpr auto vpc{oo::verticesPerCell<uint32_t>};
  constexpr auto vpq{oo::verticesPerQuad<uint32_t>};

  // Normal data can be generated implicitly by the terrain but instead of
  // being passed as vertex data the normals are saved in a texture. We have
  // explicit normal data in the LAND record so will just generate it ourselves.
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  std::array<uint8_t, vpc * vpc * 3u> normalMapData{};
  Ogre::PixelBox normalMapBox(vpc, vpc, 1, Ogre::PixelFormat::PF_BYTE_RGB,
                              normalMapData.data());

  const auto landIdOpt{getLandId(cellId)};
  if (!landIdOpt) {
    // No LAND for this cell or any of its parents. Wait for the Ogre::Terrain
    // to finish loading then delete it and return.
    if (terrainCounter) terrainCounter->wait();
    unloadTerrain(pos);
    logger->warn("[{}]: No LAND record for this terrain",
                 boost::this_fiber::get_id());
    return;
  }
  auto &landRes{oo::getResolver<record::LAND>(mResolvers)};
  record::LAND &landRec{*landRes.get(*landIdOpt)};

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
  auto layerMaps{makeDefaultLayerMaps()};
  auto layerOrders{makeDefaultLayerOrders()};

  applyBaseLayers(layerMaps, landRec);
  applyBaseLayers(layerOrders, landRec);

  applyFineLayers(layerMaps, landRec);
  applyFineLayers(layerOrders, landRec);

  logger->info("[{}]: Waiting on terrainLoad jobs...",
               boost::this_fiber::get_id());
  if (terrainCounter) oo::RenderJobManager::waitOn(terrainCounter.get());
  logger->info("[{}]: terrainLoad jobs complete!",
               boost::this_fiber::get_id());

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

  oo::JobCounter blitCounter{1};
  oo::RenderJobManager::runJob([terrain, &blitLayerMaps, &blitBoxes]() {
    auto logger{spdlog::get(oo::LOG)};
    {
      const Ogre::Box box(0u, 0u, vpq, vpq);
      blitBoxes(terrain[0]->getMaterialName(), box);
      blitLayerMaps(0);
      terrain[0]->setGlobalColourMapEnabled(true, 2u);
      terrain[0]->setGlobalColourMapEnabled(false, 2u);
      terrain[0]->_setCompositeMapRequired(true);
      logger->info("[{}]: Blit layer 0", boost::this_fiber::get_id());
    }

    {
      const Ogre::Box box(vpq - 1u, 0u, vpc, vpq);
      blitBoxes(terrain[1]->getMaterialName(), box);
      blitLayerMaps(1);
      terrain[1]->setGlobalColourMapEnabled(true, 2u);
      terrain[1]->setGlobalColourMapEnabled(false, 2u);
      terrain[1]->_setCompositeMapRequired(true);
      logger->info("[{}]: Blit layer 1", boost::this_fiber::get_id());
    }

    {
      const Ogre::Box box(0u, vpq - 1u, vpq, vpc);
      blitBoxes(terrain[2]->getMaterialName(), box);
      blitLayerMaps(2);
      terrain[2]->setGlobalColourMapEnabled(true, 2u);
      terrain[2]->setGlobalColourMapEnabled(false, 2u);
      terrain[2]->_setCompositeMapRequired(true);
      logger->info("[{}]: Blit layer 2", boost::this_fiber::get_id());
    }

    {
      const Ogre::Box box(vpq - 1u, vpq - 1u, vpc, vpc);
      blitBoxes(terrain[3]->getMaterialName(), box);
      blitLayerMaps(3);
      terrain[3]->setGlobalColourMapEnabled(true, 2u);
      terrain[3]->setGlobalColourMapEnabled(false, 2u);
      terrain[3]->_setCompositeMapRequired(true);
      logger->info("[{}]: Blit layer 3", boost::this_fiber::get_id());
    }
  }, &blitCounter);

  logger->info("[{}]: Waiting on blitCounter...", boost::this_fiber::get_id());
  blitCounter.wait();
  logger->info("[{}]: blitCounter complete!", boost::this_fiber::get_id());

  logger->info("[{}]: Creating water instance...", boost::this_fiber::get_id());
  oo::JobCounter waterCounter{1};
  oo::RenderJobManager::runJob([&]() {
    this->loadWaterPlane(pos, *cellRec);
  }, &waterCounter);
  waterCounter.wait();
  logger->info("[{}]: Created water instance.", boost::this_fiber::get_id());
}

void World::loadTerrain(oo::ExteriorCell &cell) {
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

void World::unloadTerrain(oo::ExteriorCell &cell) {
  getPhysicsWorld()->removeCollisionObject(cell.getCollisionObject());
  auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};

  const auto cellRec{cellRes.get(cell.getBaseId())};
  if (!cellRec) return;

  CellIndex pos{cellRec->grid->data.x, cellRec->grid->data.y};
  unloadTerrain(pos);
}

void World::unloadTerrain(oo::BaseId cellId) {
  auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};

  const auto cellRec{cellRes.get(cellId)};
  if (!cellRec) return;

  CellIndex pos{cellRec->grid->data.x, cellRec->grid->data.y};
  oo::JobCounter unloadDone{1};
  oo::RenderJobManager::runJob([this, pos]() {
    this->unloadTerrain(pos);
  }, &unloadDone);
  unloadDone.wait();
}

void World::unloadTerrain(CellIndex index) {
  auto x{qvm::X(index)};
  auto y{qvm::Y(index)};
  mTerrainGroup.unloadTerrain(2 * x + 0, 2 * y + 0);
  mTerrainGroup.unloadTerrain(2 * x + 1, 2 * y + 0);
  mTerrainGroup.unloadTerrain(2 * x + 0, 2 * y + 1);
  mTerrainGroup.unloadTerrain(2 * x + 1, 2 * y + 1);
  unloadWaterPlane(index);
}

void World::loadWaterPlane(CellIndex index, const record::CELL &cellRec) {
  if (auto it{mWaterPlanes.find(index)}; it != mWaterPlanes.end()) return;

  const float height{cellRec.waterHeight ? cellRec.waterHeight->data : 0.0f};
  // Position offset compensates for plane origin at its centre, not SW corner.
  const Ogre::Vector3 pos{(qvm::X(index) + 0.5f) * oo::unitsPerCell<float>,
                          (qvm::Y(index) + 0.5f) * oo::unitsPerCell<float>,
                          height};
  auto *root{mScnMgr->getRootSceneNode()};
  auto *node{root->createChildSceneNode(oo::fromBSCoordinates(pos))};

  const std::string matName{WATER_BASE_MATERIAL + mBaseId.string()};
  const std::string mgrName{WATER_MANAGER_BASE_NAME + mBaseId.string()};
  auto *entity{mScnMgr->createInstancedEntity(matName, mgrName)};
  if (!entity) return;

  entity->_getOwner()->setRenderQueueGroup(WATER_RENDER_QUEUE_GROUP);

  auto[it, _]{mWaterPlanes.try_emplace(index, node, entity)};
  const auto &waterEntry{it->second};
  waterEntry.entity->setInUse(true);
  waterEntry.node->attachObject(waterEntry.entity);
}

void World::unloadWaterPlane(CellIndex index) {
  auto it{mWaterPlanes.find(index)};
  if (it == mWaterPlanes.end()) return;
  mScnMgr->destroyInstancedEntity(it->second.entity);
  mScnMgr->destroySceneNode(it->second.node);
  mWaterPlanes.erase(it);
}

oo::ReifyRecordTrait<record::WRLD>::type
reifyRecord(const record::WRLD &refRec,
            ReifyRecordTrait<record::WRLD>::resolvers resolvers) {
  const oo::BaseId baseId{refRec.mFormId};
  std::string name{refRec.name ? refRec.name->data : ""};

  auto world{std::make_shared<oo::World>(baseId, name, resolvers)};

  return world;
}

} // namespace oo
