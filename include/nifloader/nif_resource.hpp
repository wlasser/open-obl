#ifndef OPENOBL_NIF_RESOURCE_HPP
#define OPENOBL_NIF_RESOURCE_HPP

#include "nifloader/loader.hpp"
#include <OgreResource.h>
#include <memory>

namespace Ogre {

/// \addtogroup OpenOBLNifloader
/// @{

class NifResource : public Ogre::Resource {
 public:
  NifResource(ResourceManager *creator,
              const String &name,
              ResourceHandle handle,
              const String &group,
              bool isManual = false,
              ManualResourceLoader *loader = nullptr);

  ~NifResource() override {
    unload();
  }

  using BlockGraph = oo::BlockGraph;

  BlockGraph getBlockGraph() const;

 protected:
  void loadImpl() override;
  void unloadImpl() override;

 private:
  BlockGraph mBlockGraph{};
};

using NifResourcePtr = std::shared_ptr<NifResource>;

/// @}

} // namespace Ogre

#endif // OPENOBL_NIF_RESOURCE_HPP
