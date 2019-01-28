#ifndef OPENOBLIVION_NIF_LOADER_STATE_HPP
#define OPENOBLIVION_NIF_LOADER_STATE_HPP

#include "nif/niobject.hpp"
#include <OgreMatrix4.h>

namespace oo {

// Convert the translation, rotation, and scale parameters into Ogre coordinates
// and return a combined transformation matrix.
Ogre::Matrix4 getTransform(const nif::NiAVObject &block);

} // namespace oo

#endif // OPENOBLIVION_NIF_LOADER_STATE_HPP
