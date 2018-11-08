#include "nifloader/nif_resource.hpp"
#include "settings.hpp"
#include "ogre/ogre_stream_wrappers.hpp"
#include <Ogre.h>
#include <spdlog/spdlog.h>
#include <istream>

namespace Ogre {

NifResource::NifResource(ResourceManager *creator,
                         const String &name,
                         ResourceHandle handle,
                         const String &group,
                         bool isManual,
                         ManualResourceLoader *loader)
    : Resource(creator, name, handle, group, isManual, loader) {}

NifResource::BlockGraph NifResource::getBlockGraph() const {
  return mBlockGraph;
}

void NifResource::loadImpl() {
  auto logger{spdlog::get(settings::ogreLog)};
  auto &resGrpMgr{ResourceGroupManager::getSingleton()};

  logger->info("Nif: {}", getName());

  auto dataStream{resGrpMgr.openResource(mName, mGroup, this)};
  auto dataStreamBuf{OgreDataStreambuf{dataStream}};
  std::istream is{&dataStreamBuf};

  mBlockGraph = nifloader::createBlockGraph(is);
}

void NifResource::unloadImpl() {
  mBlockGraph = nifloader::BlockGraph{};
}

} // namespace Ogre
