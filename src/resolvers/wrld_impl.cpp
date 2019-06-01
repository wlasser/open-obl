#include "mesh/mesh_manager.hpp"
#include "util/settings.hpp"
#include "wrld_impl.hpp"
#include <OgreHardwarePixelBuffer.h>
#include <OgreInstanceBatch.h>
#include <OgreInstanceManager.h>
#include <OgreInstancedEntity.h>
#include <OgreMaterialManager.h>
#include <OgreMeshManager.h>
#include <OgrePixelFormat.h>
#include <OgreRoot.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

namespace oo {

void writeNormals(Ogre::PixelBox dst, const record::LAND &rec) {
  constexpr auto vpc{oo::verticesPerCell<uint32_t>};

  if (!rec.normals) {
    // No normal data, use vertical normals
    for (std::size_t y = 0; y < vpc; ++y) {
      for (std::size_t x = 0; x < vpc; ++x) {
        dst.setColourAt(Ogre::ColourValue{0.0f, 1.0f, 0.0f}, x, y, 0);
      }
    }
    return;
  }

  for (std::size_t y = 0; y < vpc; ++y) {
    for (std::size_t x = 0; x < vpc; ++x) {
      auto[nx, ny, nz]{rec.normals->data[y * vpc + x]};
      auto n{oo::fromBSCoordinates(Ogre::Vector3(nx, ny, nz))};
      n.normalise();
      dst.setColourAt(Ogre::ColourValue{n.x, n.y, n.z}, x, y, 0);
    }
  }
}

void writeVertexCols(Ogre::PixelBox dst, const record::LAND &rec) {
  constexpr auto vpc{oo::verticesPerCell<uint32_t>};

  if (!rec.colors) {
    // No vertex colours, use white so textures actually shows up.
    std::fill(dst.data, dst.data + vpc * vpc * 3u, 255u);
    return;
  }

  for (std::size_t y = 0; y < vpc; ++y) {
    for (std::size_t x = 0; x < vpc; ++x) {
      auto[r, g, b]{rec.colors->data[y * vpc + x]};
      Ogre::ColourValue col(r / 255.0f, g / 255.0f, b / 255.0f);
      dst.setColourAt(col, x, y, 0);
    }
  }
}

LayerMaps makeDefaultLayerMaps() {
  LayerMaps layerMaps;
  std::generate(layerMaps.begin(), layerMaps.end(), []() -> LayerMap {
    LayerMap layers{};
    layers[oo::BaseId{0}].fill(255);
    return layers;
  });

  return layerMaps;
}

LayerOrders makeDefaultLayerOrders() {
  LayerOrders layerOrders;
  std::generate(layerOrders.begin(), layerOrders.end(), []() -> LayerOrder {
    return std::vector{oo::BaseId{0}};
  });

  return layerOrders;
}

void applyBaseLayers(LayerMaps &layerMaps, const record::LAND &rec) {
  // Find all the quadrant base textures, overwriting the default layer.
  for (const record::BTXT &quadrantTexture : rec.quadrantTexture) {
    const int quadrant{quadrantTexture.data.quadrant};
    oo::BaseId id{quadrantTexture.data.id};
    layerMaps[quadrant].clear();
    layerMaps[quadrant][id].fill(255);
  }
}

void applyBaseLayers(LayerOrders &layerOrders, const record::LAND &rec) {
  // Find all the quadrant base textures, overwriting the default layer.
  for (const record::BTXT &quadrantTexture : rec.quadrantTexture) {
    const int quadrant{quadrantTexture.data.quadrant};
    oo::BaseId id{quadrantTexture.data.id};
    layerOrders[quadrant][0] = id;
  }
}

void applyFineLayers(LayerMaps &layerMaps, const record::LAND &rec) {
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

void applyFineLayers(LayerOrders &layerOrders, const record::LAND &rec) {
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

void applyLayerMap(Ogre::Terrain *quad, LayerMap &layerMap,
                   const LayerOrder &layerOrder) {
  constexpr auto vpq{oo::verticesPerQuad<uint32_t>};
  constexpr auto vpqm1{oo::verticesPerQuad<float> - 1.0f};

  for (uint8_t layerNum = 1; layerNum < layerOrder.size(); ++layerNum) {
    const auto id{layerOrder[layerNum]};
    const auto &srcMap{layerMap[id]};
    auto *dstMap{quad->getLayerBlendMap(layerNum)};
    for (std::size_t y = 0; y < vpq; ++y) {
      for (std::size_t x = 0; x < vpq; ++x) {
        const float opacity{srcMap[vpq * y + x] / 255.0f};
        std::size_t s{}, t{};
        dstMap->convertUVToImageSpace(x / vpqm1, y / vpqm1, &s, &t);
        dstMap->setBlendValue(s, t, opacity);
      }
    }
    dstMap->update();
  }
}

void blitNormals(std::string matName, Ogre::PixelBox src, Ogre::Box region) {
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  auto np{texMgr.getByName(matName.append("normal"), oo::RESOURCE_GROUP)};
  np->getBuffer()->blitFromMemory(src.getSubVolume(region, true));
}

void blitVertexCols(std::string matName, Ogre::PixelBox src, Ogre::Box region) {
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  auto vcp{texMgr.getByName(matName.append("vertexcolor"), oo::RESOURCE_GROUP)};
  vcp->getBuffer()->blitFromMemory(src.getSubVolume(region, true));
}

void blitTerrainTextures(Ogre::Terrain *quad,
                         LayerMap &layerMap,
                         const LayerOrder &layerOrder,
                         Ogre::PixelBox normals,
                         Ogre::PixelBox vertexCols,
                         Ogre::Box region) {
  const std::string &matName{quad->getMaterialName()};
  oo::blitNormals(matName, normals, region);
  oo::blitVertexCols(matName, vertexCols, region);
  oo::applyLayerMap(quad, layerMap, layerOrder);
  quad->setGlobalColourMapEnabled(true, 2u);
  quad->setGlobalColourMapEnabled(false, 2u);
  quad->_setCompositeMapRequired(true);
}

void emplaceTerrainTexture(Ogre::StringVector &list, std::string texName) {
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

void
setTerrainHeights(ImportDataArray &importData, const record::raw::VHGT &rec) {
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

  // Because of the offsets it's much easier to treat the entire cell as a
  // whole and then pull out the quadrants afterwards.
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
    const auto *src{&tmp[vpc * j]};
    std::memcpy(&importData[0].inputFloat[vpq * j], src, vpq * sizeof(float));
  }

  for (std::size_t j = 0; j < vpq; ++j) {
    const auto *src{&tmp[vpc * j + (vpq - 1u)]};
    std::memcpy(&importData[1].inputFloat[vpq * j], src, vpq * sizeof(float));
  }

  for (std::size_t j = 0; j < vpq; ++j) {
    const auto *src{&tmp[vpc * (j + vpq - 1u)]};
    std::memcpy(&importData[2].inputFloat[vpq * j], src, vpq * sizeof(float));
  }

  for (std::size_t j = 0; j < vpq; ++j) {
    const auto *src{&tmp[vpc * (j + vpq - 1u) + (vpq - 1u)]};
    std::memcpy(&importData[3].inputFloat[vpq * j], src, vpq * sizeof(float));
  }
}

//===----------------------------------------------------------------------===//
// WorldImpl definitions
//===----------------------------------------------------------------------===//

World::WorldImpl::WorldImpl(oo::BaseId baseId,
                            std::string name,
                            Resolvers resolvers)
    : mBaseId(baseId), mName(std::move(name)), mResolvers(std::move(resolvers)),
      mScnMgr(Ogre::Root::getSingleton().createSceneManager(
          "oo::DeferredSceneManager"),
              [](Ogre::SceneManager *p) {
                auto root{Ogre::Root::getSingletonPtr()};
                if (root) root->destroySceneManager(p);
              }),
      mTerrainGroup(mScnMgr.get(), Ogre::Terrain::Alignment::ALIGN_X_Z,
                    oo::verticesPerQuad<uint16_t>,
                    oo::metersPerUnit<Ogre::Real> * oo::unitsPerQuad<float>),
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

  makeWaterPlane();
  makeWaterMaterial();
  makeWaterInstanceManager();

  spdlog::get(oo::LOG)->info("WRLD {}: Making physics world...", baseId);
  makePhysicsWorld();
  boost::this_fiber::yield();

  spdlog::get(oo::LOG)->info("WRLD {}: Making cell grid...", baseId);
  makeCellGrid();
  makeDistantCellGrid();
  boost::this_fiber::yield();
}

World::WorldImpl::~WorldImpl() {
  mTerrainGroup.removeAllTerrains();
}

oo::BaseId World::WorldImpl::getBaseId() const {
  return mBaseId;
}

std::string World::WorldImpl::getName() const {
  return mName;
}

void World::WorldImpl::setName(std::string name) {
  mName = std::move(name);
}

gsl::not_null<Ogre::SceneManager *> World::WorldImpl::getSceneManager() const {
  return gsl::make_not_null(mScnMgr.get());
}

gsl::not_null<World::PhysicsWorld *> World::WorldImpl::getPhysicsWorld() const {
  return gsl::make_not_null(mPhysicsWorld.get());
}

std::shared_ptr<oo::JobCounter>
World::WorldImpl::loadTerrain(CellIndex index, bool async) {
  return async ? loadTerrainAsyncImpl(index) : loadTerrainSyncImpl(index);
}

void World::WorldImpl::loadTerrain(oo::ExteriorCell &cell) {
  loadTerrain(cell.getBaseId(), false);

  const auto cellRec{getCell(cell.getBaseId())};
  if (!cellRec) return;

  CellIndex pos{cellRec->grid->data.x, cellRec->grid->data.y};
  std::array<Ogre::Terrain *, 4u> terrain{getTerrainQuads(pos)};
  cell.setTerrain(terrain);
  getPhysicsWorld()->addCollisionObject(cell.getCollisionObject());
}

void World::WorldImpl::unloadTerrain(oo::ExteriorCell &cell) {
  getPhysicsWorld()->removeCollisionObject(cell.getCollisionObject());
  unloadTerrain(cell.getBaseId());
}

void World::WorldImpl::loadTerrain(oo::BaseId cellId, bool async) {
  auto logger{spdlog::get(oo::LOG)};
  const auto fiberId{boost::this_fiber::get_id()};

  const auto cellRec{getCell(cellId)};
  if (!cellRec) return;

  CellIndex pos{cellRec->grid->data.x, cellRec->grid->data.y};

  if (isTerrainLoaded(pos)) {
    logger->info("[{}]: CELL {} terrain is already loaded", fiberId, cellId);
    return;
  }

  auto terrainCounter{loadTerrain(pos, async)};
  if (terrainCounter) {
    logger->info("[{}]: CELL {} terrain load started", fiberId, cellId);
  }

  const auto landIdOpt{getLandId(cellId)};
  if (!landIdOpt) {
    // No LAND for this cell or any of its parents. Wait for the Ogre::Terrain
    // to finish loading then delete it and return.
    if (terrainCounter) terrainCounter->wait();
    unloadTerrain(pos);
    logger->warn("[{}]: CELL {} and its ancestors have no LAND record",
                 fiberId, cellId);
    return;
  }
  auto &landRes{oo::getResolver<record::LAND>(mResolvers)};
  const record::LAND &landRec{*landRes.get(*landIdOpt)};

  constexpr auto vpc{oo::verticesPerCell<uint32_t>};
  constexpr auto vpq{oo::verticesPerQuad<uint32_t>};

  // Normal data can be generated implicitly by the terrain but instead of
  // being passed as vertex data the normals are saved in a texture. We have
  // explicit normal data in the LAND record so will just generate it ourselves.
  std::array<uint8_t, vpc * vpc * 3u> normalsData{};
  Ogre::PixelBox normals(vpc, vpc, 1, Ogre::PixelFormat::PF_BYTE_RGB,
                         normalsData.data());
  oo::writeNormals(normals, landRec);

  // Vertex colours are also stored in a texture instead of being passed as
  // vertex data.
  std::array<uint8_t, vpc * vpc * 3u> vertexColsData{};
  Ogre::PixelBox vertexCols(vpc, vpc, 1, Ogre::PixelFormat::PF_BYTE_RGB,
                            vertexColsData.data());
  oo::writeVertexCols(vertexCols, landRec);

  // Build the base texture layer and blend layers.
  auto layerMaps{oo::makeDefaultLayerMaps()};
  auto layerOrders{oo::makeDefaultLayerOrders()};

  oo::applyBaseLayers(layerMaps, landRec);
  oo::applyBaseLayers(layerOrders, landRec);

  oo::applyFineLayers(layerMaps, landRec);
  oo::applyFineLayers(layerOrders, landRec);

  if (terrainCounter) {
    logger->info("[{}]: CELL {} terrain load waiting", fiberId, cellId);
    oo::RenderJobManager::waitOn(terrainCounter.get());
    logger->info("[{}]: CELL {} terrain load finished", fiberId, cellId);
  }

  std::array<Ogre::Terrain *, 4u> terrain{getTerrainQuads(pos)};
  if (std::any_of(terrain.begin(), terrain.end(), std::logical_not<>{})) {
    logger->error("Null terrain at ({}, {})", qvm::X(pos), qvm::Y(pos));
    throw std::runtime_error("Null terrain");
  }

  std::array<Ogre::Box, 4u> regions{
      Ogre::Box(0u, 0u, vpq, vpq),
      Ogre::Box(vpq - 1u, 0u, vpc, vpq),
      Ogre::Box(0u, vpq - 1u, vpq, vpc),
      Ogre::Box(vpq - 1u, vpq - 1u, vpc, vpc)
  };

  logger->info("[{}]: CELL {} terrain blit started", fiberId, cellId);
  oo::JobCounter blitCounter{1};
  oo::RenderJobManager::runJob([&]() {
    for (std::size_t i = 0; i < 4; ++i) {
      oo::blitTerrainTextures(terrain[i], layerMaps[i], layerOrders[i],
                              normals, vertexCols, regions[i]);
    }
  }, &blitCounter);
  blitCounter.wait();
  logger->info("[{}]: CELL {} terrain blit finished", fiberId, cellId);

  logger->info("[{}]: CELL {} water creation started", fiberId, cellId);
  oo::JobCounter waterCounter{1};
  oo::RenderJobManager::runJob([&]() {
    this->loadWaterPlane(pos, *cellRec);
  }, &waterCounter);
  waterCounter.wait();
  logger->info("[{}]: CELL {} water creation finished", fiberId, cellId);
}

void World::WorldImpl::unloadTerrain(oo::BaseId cellId) {
  const auto cellRec{getCell(cellId)};
  if (!cellRec) return;

  CellIndex pos{cellRec->grid->data.x, cellRec->grid->data.y};
  oo::JobCounter unloadDone{1};
  oo::RenderJobManager::runJob([this, pos]() {
    this->unloadTerrain(pos);
  }, &unloadDone);
  unloadDone.wait();
}

void World::WorldImpl::unloadTerrain(CellIndex index) {
  auto x{qvm::X(index)};
  auto y{qvm::Y(index)};
  mTerrainGroup.unloadTerrain(2 * x + 0, 2 * y + 0);
  mTerrainGroup.unloadTerrain(2 * x + 1, 2 * y + 0);
  mTerrainGroup.unloadTerrain(2 * x + 0, 2 * y + 1);
  mTerrainGroup.unloadTerrain(2 * x + 1, 2 * y + 1);
  unloadWaterPlane(index);
}

void World::WorldImpl::updateAtmosphere(const oo::chrono::minutes &time) {
  mAtmosphere.update(time);
}

tl::optional<const record::CELL &>
World::WorldImpl::getCell(oo::BaseId cellId) const {
  const auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};
  return cellRes.get(cellId);
}

std::shared_ptr<oo::JobCounter>
World::WorldImpl::loadTerrainAsyncImpl(CellIndex index) {
  auto x{qvm::X(index)}, y{qvm::Y(index)};
  auto jc{std::make_shared<oo::JobCounter>(4)};
  auto logger{spdlog::get(oo::LOG)};

  oo::RenderJobManager::runJob([&group = mTerrainGroup, x, y, logger]() {
    logger->info("[{}]: Loading ({}, {}) terrain quad 0",
                 boost::this_fiber::get_id(), x, y);
    group.loadTerrain(2 * x + 0, 2 * y + 0, true);
  }, jc.get());

  oo::RenderJobManager::runJob([&group = mTerrainGroup, x, y, logger]() {
    logger->info("[{}]: Loading ({}, {}) terrain quad 1",
                 boost::this_fiber::get_id(), x, y);
    group.loadTerrain(2 * x + 1, 2 * y + 0, true);
  }, jc.get());

  oo::RenderJobManager::runJob([&group = mTerrainGroup, x, y, logger]() {
    logger->info("[{}]: Loading ({}, {}) terrain quad 2",
                 boost::this_fiber::get_id(), x, y);
    group.loadTerrain(2 * x + 0, 2 * y + 1, true);
  }, jc.get());

  oo::RenderJobManager::runJob([&group = mTerrainGroup, x, y, logger]() {
    logger->info("[{}]: Loading ({}, {}) terrain quad 3",
                 boost::this_fiber::get_id(), x, y);
    group.loadTerrain(2 * x + 1, 2 * y + 1, true);
  }, jc.get());

  return jc;
}

std::shared_ptr<oo::JobCounter>
World::WorldImpl::loadTerrainSyncImpl(CellIndex index) {
  auto x{qvm::X(index)}, y{qvm::Y(index)};
  mTerrainGroup.loadTerrain(2 * x + 0, 2 * y + 0, true);
  mTerrainGroup.loadTerrain(2 * x + 1, 2 * y + 0, true);
  mTerrainGroup.loadTerrain(2 * x + 0, 2 * y + 1, true);
  mTerrainGroup.loadTerrain(2 * x + 1, 2 * y + 1, true);

  return nullptr;
}

std::array<Ogre::Terrain *, 4u>
World::WorldImpl::getTerrainQuads(CellIndex index) const {
  return {
      mTerrainGroup.getTerrain(2 * qvm::X(index) + 0, 2 * qvm::Y(index) + 0),
      mTerrainGroup.getTerrain(2 * qvm::X(index) + 1, 2 * qvm::Y(index) + 0),
      mTerrainGroup.getTerrain(2 * qvm::X(index) + 0, 2 * qvm::Y(index) + 1),
      mTerrainGroup.getTerrain(2 * qvm::X(index) + 1, 2 * qvm::Y(index) + 1)
  };
}

bool World::WorldImpl::isTerrainLoaded(CellIndex index) const noexcept {
  // Sufficient to check just one quadrant because marked as loaded before
  // loading is complete?
  auto *terrain{mTerrainGroup.getTerrain(2 * qvm::X(index), 2 * qvm::Y(index))};
  return terrain && terrain->isLoaded();
}

void World::WorldImpl::setDefaultImportData() {
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

tl::optional<oo::BaseId>
World::WorldImpl::getLandId(oo::BaseId cellId) const noexcept {
  return getLandId(cellId, mBaseId);
}

tl::optional<oo::BaseId>
World::WorldImpl::getLandId(oo::BaseId cellId,
                            oo::BaseId wrldId) const noexcept {
  auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};
  auto &wrldRes{oo::getResolver<record::WRLD>(mResolvers)};

  const record::WRLD &wrldRec{*wrldRes.get(wrldId)};

  // If no parent worldspace then expect this CELL to have its own LAND,
  // otherwise lookup the CELL at the same position in the parent worldspace and
  // use its LAND record instead.

  if (!wrldRec.parentWorldspace) {
    cellRes.loadTerrain(cellId, oo::getResolvers<record::LAND>(mResolvers));
    if (auto landId{cellRes.getLandId(cellId)}) return landId;

    spdlog::get(oo::LOG)->warn("CELL {} in WRLD {} has no LAND record and "
                               "the WRLD has no parent worldspace",
                               cellId, wrldId);
    return tl::nullopt;
  }

  // TODO: Find a way to use tl::optional's monadic interface here. Though maybe
  //       tl::expected is a better fit?

  const auto cellOpt{cellRes.get(cellId)};
  if (!cellOpt) {
    spdlog::get(oo::LOG)->warn("CELL {} in WRLD {} not found",
                               cellId, wrldId);
    return tl::nullopt;
  }

  const auto gridOpt{cellOpt->grid};
  if (!gridOpt) {
    spdlog::get(oo::LOG)->warn("CELL {} in WRLD {} has no XCLC record",
                               cellId, wrldId);
    return tl::nullopt;
  }
  const oo::CellIndex pos{gridOpt->data.x, gridOpt->data.y};

  const oo::BaseId parentWrldId{wrldRec.parentWorldspace->data};
  if (!wrldRes.contains(parentWrldId)) {
    spdlog::get(oo::LOG)->warn("Parent WRLD {} of WRLD {} not found",
                               parentWrldId, wrldId);
    return tl::nullopt;
  }

  // TODO: Add a builtin function for testing if a WRLD is loaded.
  if (!wrldRes.getCells(parentWrldId)) {
    wrldRes.load(parentWrldId, oo::getResolvers<record::CELL>(mResolvers));
  }

  const auto parentCellIdOpt{wrldRes.getCell(parentWrldId, pos)};
  if (!parentCellIdOpt) {
    spdlog::get(oo::LOG)->warn("Parent of CELL {} in WRLD {} not found",
                               cellId, parentWrldId);
    return tl::nullopt;
  }
  const oo::BaseId parentCellId{*parentCellIdOpt};

  // Try to load the LAND record of this parent cell, and if that fails keep
  // going up through parent worldspaces until we succeed or run into an error.
  auto landId{getLandId(parentCellId, parentWrldId)};
  if (!landId) {
    spdlog::get(oo::LOG)->warn("Neither CELL {} nor parent CELL {} has a "
                               "LAND record", cellId, parentCellId);
    return tl::nullopt;
  }

  return landId;
}

tl::optional<oo::BaseId> World::WorldImpl::getWatrId() const noexcept {
  return getWatrId(mBaseId);
}

tl::optional<oo::BaseId>
World::WorldImpl::getWatrId(oo::BaseId wrldId) const noexcept {
  auto &wrldRes{oo::getResolver<record::WRLD>(mResolvers)};
  const record::WRLD &wrldRec{*wrldRes.get(wrldId)};

  if (!wrldRec.parentWorldspace) {
    // TODO: Use tl::optional everywhere.
    return wrldRec.water ? tl::optional{wrldRec.water->data} : tl::nullopt;
  }

  return getWatrId(wrldRec.parentWorldspace->data);
}

oo::BaseId World::WorldImpl::getAncestorWrldId() const noexcept {
  return getAncestorWrldId(mBaseId);
}

oo::BaseId
World::WorldImpl::getAncestorWrldId(oo::BaseId wrldId) const noexcept {
  auto &wrldRes{oo::getResolver<record::WRLD>(mResolvers)};
  const auto &wrldRec{*wrldRes.get(wrldId)};
  if (wrldRec.parentWorldspace) {
    return getAncestorWrldId(wrldRec.parentWorldspace->data);
  }
  return wrldId;
}

void World::WorldImpl::makeWaterPlane() const {
  auto &meshMgr{Ogre::MeshManager::getSingleton()};
  if (!meshMgr.resourceExists(WATER_MESH_NAME, oo::RESOURCE_GROUP)) {
    meshMgr.createPlane(WATER_MESH_NAME, oo::RESOURCE_GROUP,
                        Ogre::Plane(Ogre::Vector3::UNIT_Y, /*height*/0.0f),
                        oo::unitsPerCell<float> * oo::metersPerUnit<float>,
                        oo::unitsPerCell<float> * oo::metersPerUnit<float>,
                        1, 1, true, 1u, 1.0f, 1.0f,
                        Ogre::Vector3::UNIT_Z);
  }
}

Ogre::MaterialPtr World::WorldImpl::makeWaterMaterial() const {
  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  const auto waterMatName{WATER_BASE_MATERIAL + getBaseId().string()};

  if (matMgr.resourceExists(waterMatName, oo::RESOURCE_GROUP)) {
    return matMgr.getByName(waterMatName, oo::RESOURCE_GROUP);
  }

  auto baseMatPtr{matMgr.getByName(WATER_BASE_MATERIAL, oo::SHADER_GROUP)};
  auto matPtr{baseMatPtr->clone(waterMatName, /*changeGroup=*/true,
                                oo::RESOURCE_GROUP)};

  if (const auto watrIdOpt{getWatrId()}) {
    const auto &watrRes{oo::getResolver<record::WATR>(mResolvers)};
    if (const auto watrOpt{watrRes.get(*watrIdOpt)}) {
      const auto &texFile{watrOpt->textureFilename};
      oo::Path watrBasePath{texFile && !texFile->data.empty()
                            ? texFile->data : "water/water00.dds"};
      oo::Path watrPath{oo::Path{"textures"} / std::move(watrBasePath)};
      Ogre::AliasTextureNamePairList layers{
          {"diffuse", watrPath.c_str()}
      };
      matPtr->applyTextureAliases(layers, true);
    } else {
      spdlog::get(oo::LOG)->warn("WRLD {}: WATR record {} does not exist",
                                 getBaseId(), *watrIdOpt);
    }
  } else {
    spdlog::get(oo::LOG)->warn("WRLD {}: No NAM2 record in this or any "
                               "ancestors", getBaseId());
  }

  matPtr->load();
  return matPtr;
}

void World::WorldImpl::makeWaterInstanceManager() const {
  auto *instMgr{mScnMgr->createInstanceManager(
      WATER_MANAGER_BASE_NAME + mBaseId.string(),
      WATER_MESH_NAME, oo::RESOURCE_GROUP,
      Ogre::InstanceManager::InstancingTechnique::HWInstancingBasic,
      /*instancesPerBatch*/32)};
  instMgr->setSetting(Ogre::InstanceManager::BatchSettingId::CAST_SHADOWS,
                      false);
}

void
World::WorldImpl::loadWaterPlane(CellIndex index, const record::CELL &cellRec) {
  //C++20: if (!mWaterPlanes.contains(index)) return;
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

void World::WorldImpl::unloadWaterPlane(CellIndex index) {
  auto it{mWaterPlanes.find(index)};
  if (it == mWaterPlanes.end()) return;
  mScnMgr->destroyInstancedEntity(it->second.entity);
  mScnMgr->destroySceneNode(it->second.node);
  mWaterPlanes.erase(it);
}

oo::DistantChunk
World::WorldImpl::makeChunk(oo::ChunkIndex chunkIndex) {
  const auto wrldId{getAncestorWrldId()};
  const std::string baseName{oo::getChunkBaseName(wrldId, chunkIndex)};
  const std::string meshPath{oo::getChunkMeshPath(wrldId, chunkIndex).c_str()};

  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  const std::string matName{CHUNK_BASE_MATERIAL + baseName};

  if (!matMgr.resourceExists(matName, oo::RESOURCE_GROUP)) {
    auto matPtr{matMgr.getByName(CHUNK_BASE_MATERIAL, oo::SHADER_GROUP)};
    auto newMatPtr{matPtr->clone(matName, /*changeGroup=*/true,
                                 oo::RESOURCE_GROUP)};

    std::string diffPath{oo::getChunkDiffusePath(wrldId, chunkIndex).c_str()};
    std::string normPath{oo::getChunkNormalPath(wrldId, chunkIndex).c_str()};

    auto *pass{newMatPtr->getTechnique(0)->getPass(0)};
    pass->removeAllTextureUnitStates();
    pass->createTextureUnitState(diffPath);
    pass->createTextureUnitState(normPath);

    const auto &vsParams{pass->getVertexProgramParameters()};
    const auto &gameSettings{oo::GameSettings::getSingleton()};
    const auto diam{gameSettings.get<unsigned>("General.uGridDistantCount", 5)};
    vsParams->setNamedConstant("gridDistantCount", static_cast<int>(diam));
  }

  auto matPtr{matMgr.getByName(matName, oo::RESOURCE_GROUP)};

  auto *node{mScnMgr->getRootSceneNode()->createChildSceneNode()};
  oo::insertRawNif(meshPath, oo::RESOURCE_GROUP, matPtr, getSceneManager(),
                   gsl::make_not_null(node));
  return {node, std::move(matPtr)};
}

void World::WorldImpl::makeDistantCellGrid() {
  auto &wrldRes{oo::getResolver<record::WRLD>(mResolvers)};
  const auto wrldId{getAncestorWrldId()};
  const auto &wrldRec{*wrldRes.get(wrldId)};
  // Worldspace bounds, in units.
  const auto[x0, y0]{wrldRec.bottomLeft.data};
  const auto[x1, y1]{wrldRec.topRight.data};

  // Worldspace bounds, in chunks
  const int i0 = std::floor(x0 / unitsPerChunk<float>);
  const int i1 = std::floor(x1 / unitsPerChunk<float>);
  const int j0 = std::floor(y0 / unitsPerChunk<float>);
  const int j1 = std::floor(y1 / unitsPerChunk<float>);

  for (int i = i0; i < i1; ++i) {
    for (int j = j0; j < j1; ++j) {
      oo::ChunkIndex index{i, j};
      mDistantChunks.emplace(index, makeChunk(index));
    }
  }
}

void World::WorldImpl::makeCellGrid() {
  auto &wrldRes{oo::getResolver<record::WRLD>(mResolvers)};

  // Number of cells to load before yielding this fiber. We have a *lot* of
  // cells to load, and yielding after every cell is slow.
  constexpr unsigned CELLS_PER_YIELD{64};
  // Increment this every cell load and reset to zero once we yield.
  unsigned cellsAfterYield{0};

  const auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};
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

    oo::setTerrainHeights(importData, heightRec);

    auto &terrainOpts{Ogre::TerrainGlobalOptions::getSingleton()};
    const auto matGen{terrainOpts.getDefaultMaterialGenerator()};
    for (auto &data : importData) {
      data.layerDeclaration = matGen->getLayerDeclaration();
    }

    auto layerOrders{oo::makeDefaultLayerOrders()};
    oo::applyBaseLayers(layerOrders, *landOpt);
    oo::applyFineLayers(layerOrders, *landOpt);

    for (std::size_t i = 0; i < 4; ++i) {
      for (auto id : layerOrders[i]) {
        auto &layer{importData[i].layerList.emplace_back()};
        layer.worldSize = 1.0f;

        if (const auto ltexOpt{ltexRes.get(id)}) {
          const oo::Path basePath{ltexOpt->textureFilename.data};
          emplaceTerrainTexture(layer.textureNames, basePath.c_str());
        } else {
          emplaceTerrainTexture(layer.textureNames, "terrainhddirt01.dds");
        }
      }
    }

    // Note: The success of getLandId() implies that the cell record exists.
    const auto gridOpt{cellRes.get(cellId)->grid};
    if (!gridOpt) {
      spdlog::get(oo::LOG)->warn("CELL {} in WRLD {} has no XCLC record",
                                 cellId, mBaseId);
      continue;
    }

    const auto x{gridOpt->data.x}, y{gridOpt->data.y};
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

void World::WorldImpl::makePhysicsWorld() {
  const auto &cellRes{oo::getResolver<record::CELL>(mResolvers)};
  const auto &bulletConf{cellRes.getBulletConfiguration()};
  mPhysicsWorld = bulletConf.makeDynamicsWorld();
}

} // namespace oo