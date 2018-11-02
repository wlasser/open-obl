#ifndef OPENOBLIVION_NIF_LOADER_STATE_HPP
#define OPENOBLIVION_NIF_LOADER_STATE_HPP

#include "nifloader/loader.hpp"
#include <boost/graph/adjacency_list.hpp>

namespace nifloader {

// Convert the translation, rotation, and scale parameters into Ogre coordinates
// and return a combined transformation matrix.
Ogre::Matrix4 getTransform(const nif::NiAVObject &block);

} // namespace nifloader

#endif // OPENOBLIVION_NIF_LOADER_STATE_HPP
