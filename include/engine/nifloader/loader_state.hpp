#ifndef OPENOBLIVION_ENGINE_NIF_LOADER_STATE_HPP
#define OPENOBLIVION_ENGINE_NIF_LOADER_STATE_HPP

#include "engine/nifloader/loader.hpp"
#include <boost/graph/adjacency_list.hpp>

namespace engine::nifloader {

// Convert the translation, rotation, and scale parameters into Ogre coordinates
// and return a combined transformation matrix.
Ogre::Matrix4 getTransform(const nif::NiAVObject &block);

} // namespace engine::nifloader

#endif // OPENOBLIVION_ENGINE_NIF_LOADER_STATE_HPP
