#include <OgreShaderGenerator.h>
#include "app.hpp"

void App::setup() {
  OgreBites::ApplicationContext::setup();

  auto archiveManager = Ogre::ArchiveManager::getSingletonPtr();
  auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
  auto meshManager = Ogre::MeshManager::getSingletonPtr();
  auto materialManager = Ogre::MaterialManager::getSingletonPtr();

  archiveManager->addArchiveFactory(bsaArchiveFactory.get());
  resourceGroupManager->addResourceLocation(
      "./Data/Oblivion - Textures - Compressed.bsa", "BSA");
  resourceGroupManager->declareResource(
      "textures/characters/imperial/headhuman.dds", "Texture");
  resourceGroupManager->initialiseAllResourceGroups();

  addInputListener(this);

  archiveManager
      ->load("./Data/Oblivion - Textures - Compressed.bsa", "BSA", true);

  auto root = getRoot();

  auto sceneManager = root->createSceneManager();

  auto shaderGen = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
  shaderGen->addSceneManager(sceneManager);

  auto light = sceneManager->createLight("MainLight");
  auto lightNode = sceneManager->getRootSceneNode()->createChildSceneNode();
  lightNode->setPosition(0, 0, 15);
  lightNode->attachObject(light);

  auto cameraNode = sceneManager->getRootSceneNode()->createChildSceneNode();
  cameraNode->setPosition(0, 0, 10);
  cameraNode->lookAt(Ogre::Vector3(0, 0, -1), Ogre::Node::TS_PARENT);
  auto camera = sceneManager->createCamera("MainCamera");
  camera->setNearClipDistance(1);
  camera->setAutoAspectRatio(true);
  cameraNode->attachObject(camera);
  getRenderWindow()->addViewport(camera);

  Ogre::Plane texturePlane{Ogre::Vector3::UNIT_Z, 0};
  meshManager->createPlane("texturePlane",
                           Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                           texturePlane,
                           4, 4, 4, 4,
                           true,
                           1, 1.0f, 1.0f,
                           Ogre::Vector3::UNIT_Y);
  auto texturePlaneMaterial = materialManager->create(
      "texturePlaneMaterial",
      Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
  {
    auto technique = texturePlaneMaterial->getTechnique(0);
    auto pass = technique->getPass(0);
    pass->setAmbient(0.5f, 0.5, 0.5f);
    pass->setDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
    auto textureUnit = pass->createTextureUnitState(
        "textures/characters/imperial/headhuman.dds");
  }
  texturePlaneMaterial->load();

  auto texturePlaneEntity = sceneManager->createEntity("texturePlane");
  texturePlaneEntity->setMaterialName("texturePlaneMaterial");
  auto texturePlaneEntityNode =
      sceneManager->getRootSceneNode()->createChildSceneNode();
  texturePlaneEntityNode->attachObject(texturePlaneEntity);
}

bool App::keyPressed(const OgreBites::KeyboardEvent &event) {
  if (event.keysym.sym == OgreBites::SDLK_ESCAPE) {
    getRoot()->queueEndRendering();
  }

  return true;
}