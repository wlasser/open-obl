#include "ogre/text_resource.hpp"
#include <OgreResourceGroupManager.h>
#include <istream>

namespace Ogre {

TextResource::TextResource(ResourceManager *creator,
                           const String &name,
                           ResourceHandle handle,
                           const String &group,
                           bool isManual,
                           ManualResourceLoader *loader)
    : Resource(creator, name, handle, group, isManual, loader) {}

std::string TextResource::getString() const {
  return mString;
}

void TextResource::loadImpl() {
  auto dataStream
      {ResourceGroupManager::getSingletonPtr()->openResource(mName, mGroup)};
  mString = dataStream->getAsString();
}

void TextResource::unloadImpl() {}

} // namespace Ogre