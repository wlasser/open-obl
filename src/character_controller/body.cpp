#include "character_controller/body.hpp"
#include "mesh/subentity.hpp"
#include "settings.hpp"
#include <OgreMaterialManager.h>
#include <OgrePass.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <OgreTextureUnitState.h>

namespace oo {

oo::Path getBodyPartPath(BodyParts bodyPart, bool isFemale) {
  switch (bodyPart) {
    case BodyParts::UpperBody:
      return oo::Path{isFemale ? "meshes/characters/_male/femaleupperbody.nif"
                               : "meshes/characters/_male/upperbody.nif"};
    case BodyParts::LowerBody:
      return oo::Path{isFemale ? "meshes/characters/_male/femalelowerbody.nif"
                               : "meshes/characters/_male/lowerbody.nif"};
    case BodyParts::Hand:
      return oo::Path{isFemale ? "meshes/characters/_male/femalehand.nif"
                               : "meshes/characters/_male/hand.nif"};
    case BodyParts::Foot:
      return oo::Path{isFemale ? "meshes/characters/_male/femalefoot.nif"
                               : "meshes/characters/_male/foot.nif"};
    default: return oo::Path{""};
  }
}

bool isSkinMaterial(const Ogre::MaterialPtr &mat) {
  for (auto *technique : mat->getTechniques()) {
    if (technique->getPass("skin")) return true;
  }

  return false;
}

void setSkinTextures(const Ogre::MaterialPtr &mat,
                     const std::string &diffuseName,
                     const std::string &normalName) {
  for (auto *technique : mat->getTechniques()) {
    auto *pass{technique->getPass("skin")};
    if (!pass) continue;
    for (auto *texState : pass->getTextureUnitStates()) {
      if (texState->getName() == "normal") {
        // Many races use the Imperial normal maps implicitly, so the normal map
        // might not actually exist.
        auto &texMgr{Ogre::TextureManager::getSingleton()};
        if (texMgr.resourceExists(normalName)) {
          texState->setTextureName(normalName);
          continue;
        }

        // The user is using an implicit fallback so they shouldn't expect
        // anything clever here. Drop back two directories, hopefully the one
        // named according to their race, and substitute the race name.
        auto fileBegin{normalName.rfind('/')};
        auto sexBegin{normalName.rfind('/', fileBegin)};
        if (sexBegin == std::string::npos) {
          // Not enough folders, we can still use the flat normal though.
          texState->setTextureName("textures/flat_n.dds");
          continue;
        }

        std::string fallbackNormal{"textures/characters/imperial"
                                       + normalName.substr(sexBegin)};
        if (texMgr.resourceExists(fallbackNormal)) {
          texState->setTextureName(fallbackNormal);
        } else {
          // Well we tried.
          texState->setTextureName("textures/flat_n.dds");
        }
      } else {
        texState->setTextureName(diffuseName);
      }
    }
  }
}

void setSkinTextures(oo::Entity *bodyPart, oo::BaseId raceId,
                     const record::ICON &textureRec) {
  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  const auto raceIdString{"/" + raceId.string()};

  const oo::Path diffPath{oo::Path{"textures"} / oo::Path{textureRec.data}};
  const oo::Path normPath{oo::makeNormalPath(diffPath)};
  const std::string diffName{diffPath.c_str()};
  const std::string normName{normPath.c_str()};

  for (const auto &subEntity : bodyPart->getSubEntities()) {
    const auto &baseMat{subEntity->getMaterial()};

    if (oo::isSkinMaterial(baseMat)) {
      const std::string matName{baseMat->getName() + raceIdString};
      if (matMgr.resourceExists(matName, oo::RESOURCE_GROUP)) {
        subEntity->setMaterial(matMgr.getByName(matName, oo::RESOURCE_GROUP));
      } else {
        const auto &matPtr{baseMat->clone(matName)};
        subEntity->setMaterial(matPtr);
        oo::setSkinTextures(matPtr, diffName, normName);
      }
    }
  }
}

} // namespace oo