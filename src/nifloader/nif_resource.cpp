#include "nifloader/logging.hpp"
#include "nifloader/nif_resource.hpp"
#include "ogre/ogre_stream_wrappers.hpp"
#include <OgreResourceGroupManager.h>

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
  auto &resGrpMgr{ResourceGroupManager::getSingleton()};

  oo::nifloaderLogger()->info("Nif: {}", getName());

  auto dataStream{resGrpMgr.openResource(mName, mGroup, this)};
  auto dataStreamBuf{OgreDataStreambuf{dataStream}};
  std::istream is{&dataStreamBuf};

  mBlockGraph = oo::createBlockGraph(is);
}

void NifResource::unloadImpl() {
  mBlockGraph = oo::BlockGraph{};
}

} // namespace Ogre
