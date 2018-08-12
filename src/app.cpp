#include <OgreShaderGenerator.h>
#include "app.hpp"

void App::setup() {
  OgreBites::ApplicationContext::setup();

  addInputListener(this);

  auto root = getRoot();

  auto sceneManager = root->createSceneManager();

  auto shaderGen = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
  shaderGen->addSceneManager(sceneManager);

  auto light = sceneManager->createLight("MainLight");
  auto lightNode = sceneManager->getRootSceneNode()->createChildSceneNode();
  lightNode->setPosition(0, 10, 15);
  lightNode->attachObject(light);

  auto cameraNode = sceneManager->getRootSceneNode()->createChildSceneNode();
  cameraNode->setPosition(0, 0, 15);
  cameraNode->lookAt(Ogre::Vector3(0, 0, -1), Ogre::Node::TS_PARENT);
  auto camera = sceneManager->createCamera("MainCamera");
  camera->setNearClipDistance(5);
  camera->setAutoAspectRatio(true);
  cameraNode->attachObject(camera);
  getRenderWindow()->addViewport(camera);

  auto entity = sceneManager->createEntity("Sinbad.mesh");
  auto entityNode = sceneManager->getRootSceneNode()->createChildSceneNode();
  entityNode->attachObject(entity);
}

bool App::keyPressed(const OgreBites::KeyboardEvent &event) {
  if (event.keysym.sym == OgreBites::SDLK_ESCAPE) {
    getRoot()->queueEndRendering();
  }

  return true;
}