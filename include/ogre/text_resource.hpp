#ifndef OPENOBL_OGRE_TEXT_RESOURCE_HPP
#define OPENOBL_OGRE_TEXT_RESOURCE_HPP

#include <OgreResource.h>

namespace Ogre {

class TextResource : public Ogre::Resource {
 public:
  TextResource(ResourceManager *creator,
               const String &name,
               ResourceHandle handle,
               const String &group,
               bool isManual = false,
               ManualResourceLoader *loader = nullptr);

  ~TextResource() override {
    unload();
  }

  String getString() const;

 protected:
  void loadImpl() override;
  void unloadImpl() override;

  String mString{};
};

using TextResourcePtr = std::shared_ptr<TextResource>;

} // namespace Ogre

#endif // OPENOBL_OGRE_TEXT_RESOURCE_HPP
