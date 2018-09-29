#include "engine/nifloader/mesh_loader.hpp"
#include "engine/nifloader/mesh_loader_state.hpp"
#include "engine/settings.hpp"
#include "ogre/ogre_stream_wrappers.hpp"

namespace engine::nifloader {

void MeshLoader::loadResource(Ogre::Resource *resource) {
  if (!logger) logger = spdlog::get(settings::ogreLog);
  auto mesh = dynamic_cast<Ogre::Mesh *>(resource);
  // TODO: Handle this properly
  assert(mesh != nullptr);

  logger->info("Mesh: {}", resource->getName());

  // TODO: If the mesh doesn't exist, dynamically generate a placeholder
  auto ogreDataStream = Ogre::ResourceGroupManager::getSingletonPtr()
      ->openResource(mesh->getName(), mesh->getGroup());

  auto ogreDataStreamBuffer = Ogre::OgreDataStreambuf{ogreDataStream};
  std::istream is{&ogreDataStreamBuffer};

  auto blocks = createBlockGraph(is);

  MeshLoaderState instance(mesh, blocks);
}

} // namespace engine::nifloader
