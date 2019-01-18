#ifndef OPENOBLIVION_NIFLOADER_SKELETON_LOADER_HPP
#define OPENOBLIVION_NIFLOADER_SKELETON_LOADER_HPP

#include <Ogre.h>
#include <memory>
#include <spdlog/spdlog.h>

namespace oo {

class SkeletonLoader : public Ogre::ManualResourceLoader {
 private:
  friend class SkeletonLoaderState;

  std::shared_ptr<spdlog::logger> logger{};

 public:
  void loadResource(Ogre::Resource *resource) override;
};

} // namespace oo

#endif // OPENOBLIVION_NIFLOADER_SKELETON_LOADER_HPP
