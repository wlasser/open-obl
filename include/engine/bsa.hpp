#ifndef OPENOBLIVION_ENGINE_BSA_HPP
#define OPENOBLIVION_ENGINE_BSA_HPP

#include <OgreArchive.h>
#include <OgreArchiveFactory.h>

namespace engine {

class BsaArchiveFactory : public Ogre::ArchiveFactory {
 public:
  ~BsaArchiveFactory() override = default;

  Ogre::Archive *createInstance(const Ogre::String &name,
                                bool readOnly) override;

  void destroyInstance(Ogre::Archive *ptr) override;
  const Ogre::String &getType() const override;
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_BSA_HPP
