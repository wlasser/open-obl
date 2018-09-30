#ifndef OPENOBLIVION_ENGINE_LIGHT_MANAGER_HPP
#define OPENOBLIVION_ENGINE_LIGHT_MANAGER_HPP

#include "formid.hpp"
#include "records.hpp"
#include <OgreColourValue.h>
#include <OgreLight.h>
#include <OgreSceneManager.h>
#include <string>

namespace engine {

struct LightEntry {
  using Flag = record::raw::DATA_LIGH::Flag;
  std::string modelFilename{};
  FormID sound{0u};
  FormID script{0u};
  float fadeValue{1.0f};
  float radius{300.0f};
  float falloffExponent{1.0f};
  float fov{90.0f};
  Ogre::ColourValue color{};
  Flag flags{Flag::None};
};

struct LightItemEntry : LightEntry {
  std::string name{};
  std::string iconFilename{};
  int32_t time{};
  int32_t value{};
  float weight{};
};

struct LightMesh {
  Ogre::Light *light{nullptr};
  Ogre::Entity *mesh{nullptr};
};

class LightManager {
 private:
  std::unordered_map<FormID, LightEntry> lights{};
  std::unordered_map<FormID, LightItemEntry> lightItems{};
  friend class InitialProcessor;

 public:
  LightMesh get(FormID baseID, Ogre::SceneManager *mgr) const;
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_LIGHT_MANAGER_HPP
