#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_BODY_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_BODY_HPP

#include "fs/path.hpp"
#include "mesh/entity.hpp"
#include "record/records.hpp"
#include <OgreMaterial.h>

namespace oo {

using BodyParts = record::raw::INDX_BODY;

/// Get the path to the mesh file describing the given `bodyPart`, relative to
/// `sMasterPath`.
oo::Path getBodyPartPath(BodyParts bodyPart, bool isFemale);

/// Return whether the given material represents skin.
/// Specifically, return true iff `mat` has at least one technique that has a
/// pass called 'skin'.
bool isSkinMaterial(const Ogre::MaterialPtr &mat);

/// Set the diffuse and normal textures of each 'skin' pass in the material to
/// those given.
void setSkinTextures(const Ogre::MaterialPtr &mat,
                     const std::string &diffuseName,
                     const std::string &normalName);

/// Change any skin materials to ones specific to the race, creating those
/// materials and setting their textures if they don't already exist.
void setSkinTextures(oo::Entity *bodyPart, oo::BaseId raceId,
                     const record::ICON &textureRec);

} // namespace oo

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_BODY_HPP
