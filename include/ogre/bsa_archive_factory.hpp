#ifndef OPENOBLIVION_OGRE_ARCHIVE_FACTORY_HPP
#define OPENOBLIVION_OGRE_ARCHIVE_FACTORY_HPP

#include <gsl/gsl>
#include <OgreArchive.h>
#include <OgreArchiveFactory.h>

namespace Ogre {

class BsaArchiveFactory : public Ogre::ArchiveFactory {
 public:
  ~BsaArchiveFactory() override = default;

  gsl::owner<Ogre::Archive *>
  createInstance(const Ogre::String &name, bool readOnly) override;

  void destroyInstance(gsl::owner<Ogre::Archive *> ptr) override;
  const Ogre::String &getType() const override;
};

} // namespace Ogre

#endif // OPENOBLIVION_OGRE_BSA_ARCHIVE_FACTORY_HPP
