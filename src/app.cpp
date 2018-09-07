#include "app.hpp"
#include "engine/nif_loader.hpp"
#include "engine/ogre_stream_wrappers.hpp"
#include <OgreShaderGenerator.h>
#include <iostream>
#include <istream>
#include <fstream>

void App::setup() {
  OgreBites::ApplicationContext::setup();

  auto logger = Ogre::LogManager::getSingletonPtr();
  auto archiveManager = Ogre::ArchiveManager::getSingletonPtr();
  auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
  auto nifLoader = engine::NifLoader();

  auto textureArchiveName = "./Data/Oblivion - Textures - Compressed.bsa";
  auto meshArchiveName = "./Data/Oblivion - Meshes.bsa";

  auto resourceGroup = "OOResource";

  auto robeMeshName = "meshes/clothes/robeuc01/f/robeuc01f.nif";

  resourceGroupManager->createResourceGroup(resourceGroup);

  archiveManager->addArchiveFactory(bsaArchiveFactory.get());
  resourceGroupManager->addResourceLocation(meshArchiveName, "BSA",
                                            resourceGroup);
  resourceGroupManager->addResourceLocation(textureArchiveName, "BSA",
                                            resourceGroup);

  resourceGroupManager->declareResource(robeMeshName, "Mesh",
                                        resourceGroup, &nifLoader);

  resourceGroupManager->initialiseResourceGroup(resourceGroup);

  addInputListener(this);

  auto root = getRoot();

  auto sceneManager = root->createSceneManager();
  auto rootNode = sceneManager->getRootSceneNode();

  auto shaderGen = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
  shaderGen->addSceneManager(sceneManager);

  auto light = sceneManager->createLight("MainLight");
  auto lightNode = rootNode->createChildSceneNode();
  lightNode->attachObject(light);
  lightNode->setPosition(50, 100, -100);

  cameraNode = rootNode->createChildSceneNode();
  cameraNode->setPosition(0, 0, 10);
  cameraNode->lookAt(Ogre::Vector3(0, 0, -1), Ogre::Node::TS_PARENT);
  auto camera = sceneManager->createCamera("MainCamera");
  camera->setNearClipDistance(1);
  camera->setAutoAspectRatio(true);
  cameraNode->attachObject(camera);
  getRenderWindow()->addViewport(camera);
  cameraMan = std::make_unique<OgreBites::CameraMan>(cameraNode);
  cameraMan->setStyle(OgreBites::CameraStyle::CS_FREELOOK);

  auto meshEntity = sceneManager->createEntity(robeMeshName);
  auto meshEntityNode = rootNode->createChildSceneNode();
  meshEntityNode->attachObject(meshEntity);
  meshEntityNode->setPosition(0.0f, 0.0f, 0.0f);

  auto bbMin = meshEntity->getBoundingBox().getMinimum();
  auto bbMax = meshEntity->getBoundingBox().getMaximum();
  std::clog << "(" << bbMin.x << ", " << bbMin.y << ", " << bbMin.z << ")\n";
  std::clog << "(" << bbMax.x << ", " << bbMax.y << ", " << bbMax.z << ")\n";
  cameraNode->setPosition(0, 80, -100);
  cameraNode->lookAt({0, 80, 0}, Ogre::Node::TS_WORLD);
}

bool App::keyPressed(const OgreBites::KeyboardEvent &event) {
  if (event.keysym.sym == OgreBites::SDLK_ESCAPE) {
    getRoot()->queueEndRendering();
  }
  cameraMan->keyPressed(event);
  return true;
}

bool App::keyReleased(const OgreBites::KeyboardEvent &event) {
  cameraMan->keyReleased(event);
  return true;
}

void App::frameRendered(const Ogre::FrameEvent &event) {
  cameraMan->frameRendered(event);
}

bool App::mouseMoved(const OgreBites::MouseMotionEvent &event) {
  cameraMan->mouseMoved(event);
  return true;
}

bool App::mousePressed(const OgreBites::MouseButtonEvent &event) {
  cameraMan->mousePressed(event);
  return true;
}

bool App::mouseReleased(const OgreBites::MouseButtonEvent &event) {
  cameraMan->mouseReleased(event);
  return true;
}