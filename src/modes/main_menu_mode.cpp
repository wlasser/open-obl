#include "modes/game_mode.hpp"
#include "modes/load_menu_mode.hpp"
#include "modes/loading_menu_mode.hpp"
#include "modes/main_menu_mode.hpp"

namespace oo {

MenuMode<gui::MenuType::MainMenu>::MenuMode(ApplicationContext &ctx) :
    MenuModeBase<MainMenuMode>(ctx),
    mScnMgr{ctx.getRoot().createSceneManager(SCN_MGR_TYPE, SCN_MGR_NAME)},
    mCamera{mScnMgr->createCamera(CAMERA_NAME)},
    btnContinue{getMenuCtx()->getElementWithId(2)},
    btnNew{getMenuCtx()->getElementWithId(3)},
    btnLoad{getMenuCtx()->getElementWithId(4)},
    btnOptions{getMenuCtx()->getElementWithId(5)},
    btnCredits{getMenuCtx()->getElementWithId(6)},
    btnExit{getMenuCtx()->getElementWithId(7)} {
  mScnMgr->addRenderQueueListener(ctx.getImGuiManager());
  mScnMgr->addRenderQueueListener(ctx.getOverlaySystem());

  ctx.setCamera(gsl::make_not_null(mCamera));

  const auto &gameSettings{oo::GameSettings::getSingleton()};
  auto &sndMgr{Ogre::SoundManager::getSingleton()};

  oo::Path musicBasePath{"music"};
  oo::Path musicFilename{gameSettings.get("General.SMainMenuMusic",
                                          "special/tes4title.mp3")};
  oo::Path musicPath{musicBasePath / musicFilename};

  mBackgroundMusic.emplace(sndMgr.playMusic(musicPath.c_str(),
                                            oo::RESOURCE_GROUP));
  float musicVolume{gameSettings.get("Audio.fMainMenuMusicVolume", 1.0f)};
  mBackgroundMusic->setVolume(musicVolume);
}

MenuMode<gui::MenuType::MainMenu>::~MenuMode() {
  auto *root{Ogre::Root::getSingletonPtr()};
  if (root && mScnMgr) root->destroySceneManager(mScnMgr);
}

MenuMode<gui::MenuType::MainMenu>::MenuMode(MenuMode<gui::MenuType::MainMenu> &&other) noexcept
    : MenuModeBase<MainMenuMode>(std::move(other)),
      mScnMgr(other.mScnMgr),
      mCamera(other.mCamera),
      mBackgroundMusic(other.mBackgroundMusic),
      btnContinue(other.btnContinue),
      btnNew(other.btnNew),
      btnLoad(other.btnLoad),
      btnOptions(other.btnOptions),
      btnCredits(other.btnCredits),
      btnExit(other.btnExit) {
  other.mScnMgr = nullptr;
  other.mCamera = nullptr;
}

MenuMode<gui::MenuType::MainMenu> &
MenuMode<gui::MenuType::MainMenu>::operator=(MenuMode<gui::MenuType::MainMenu> &&other) noexcept {
  if (this != &other) {
    auto tmp{std::move(other)};
    std::swap(*this, tmp);
  }
  return *this;
}

MainMenuMode::transition_t
MainMenuMode::handleEventImpl(ApplicationContext &ctx,
                              const sdl::Event &/*event*/) {
  if (btnContinue && btnContinue->is_clicked()) {
    getMenuCtx()->getOverlay()->hide();
    mBackgroundMusic->stop();
    return {false, oo::GameMode(ctx)};
  }

  if (btnLoad && btnLoad->is_clicked()) {
    getMenuCtx()->getOverlay()->setZOrder(0);
    return {false, oo::LoadMenuMode(ctx)};
  }
  if (btnExit && btnExit->is_clicked()) {
    return {true, std::nullopt};
  }

  return {false, std::nullopt};
}

void MainMenuMode::updateImpl(ApplicationContext &/*ctx*/, float /*delta*/) {
  const float transitionLength{getMenuCtx()->get_user<float>(4)};
  getMenuCtx()->set_user(0, true);
  getMenuCtx()->set_user(1, getClock() > transitionLength);
  getMenuCtx()->set_user(2, getClock() <= transitionLength);
  const float alpha{255.0f * (1.0f - getClock() / transitionLength)};
  getMenuCtx()->set_user(3, std::max(0.0f, alpha));
}

} // namespace oo
