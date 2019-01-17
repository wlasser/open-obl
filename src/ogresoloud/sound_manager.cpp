#include "ogresoloud/sound_manager.hpp"
#include "ogresoloud/wav_resource_manager.hpp"

namespace Ogre {

template<> SoundManager *Singleton<SoundManager>::msSingleton = nullptr;

SoundManager &SoundManager::getSingleton() {
  assert(msSingleton);
  return *msSingleton;
}

SoundManager *SoundManager::getSingletonPtr() {
  return msSingleton;
}

String SoundManager::errorToString(SoLoud::SOLOUD_ERRORS error) {
  switch (error) {
    case SoLoud::SO_NO_ERROR: return "NO_ERROR";
    case SoLoud::INVALID_PARAMETER: return "INVALID_PARAMETER";
    case SoLoud::FILE_NOT_FOUND: return "FILE_NOT_FOUND";
    case SoLoud::FILE_LOAD_FAILED: return "FILE_LOAD_FAILED";
    case SoLoud::DLL_NOT_FOUND: return "DLL_NOT_FOUND";
    case SoLoud::OUT_OF_MEMORY: return "OUT_OF_MEMORY";
    case SoLoud::NOT_IMPLEMENTED: return "NOT_IMPLEMENTED";
    default: [[fallthrough]];
    case SoLoud::UNKNOWN_ERROR: return "UNKNOWN_ERROR";
  }
}

Exception::ExceptionCodes
SoundManager::errorToExceptionCode(SoLoud::SOLOUD_ERRORS error) noexcept {
  switch (error) {
    case SoLoud::SO_NO_ERROR: return Exception::ERR_INTERNAL_ERROR;
    case SoLoud::INVALID_PARAMETER: return Exception::ERR_INVALIDPARAMS;
    case SoLoud::FILE_NOT_FOUND: return Exception::ERR_FILE_NOT_FOUND;
    case SoLoud::FILE_LOAD_FAILED: return Exception::ERR_INTERNAL_ERROR;
    case SoLoud::DLL_NOT_FOUND: return Exception::ERR_FILE_NOT_FOUND;
    case SoLoud::OUT_OF_MEMORY: return Exception::ERR_INTERNAL_ERROR;
    case SoLoud::NOT_IMPLEMENTED: return Exception::ERR_NOT_IMPLEMENTED;
    default: [[fallthrough]];
    case SoLoud::UNKNOWN_ERROR: return Exception::ERR_INTERNAL_ERROR;
  }
}

SoundManager::SoundManager() {
  mSoloud = std::make_unique<SoLoud::Soloud>();
  if (auto err{static_cast<SoLoud::SOLOUD_ERRORS>(mSoloud->init())}; err) {
    OGRE_EXCEPT(errorToExceptionCode(err),
                String{"Failed to initialise SoLoud: "} + errorToString(err),
                "SoundManager::SoundManager()");
  }
}

SoundHandle SoundManager::playMusic(const String &name,
                                    const String &group,
                                    float volume) const {
  auto &wavResMgr{WavResourceManager::getSingleton()};

  auto wavResPtr{wavResMgr.getByName(name, group)};
  if (!wavResPtr) {
    OGRE_EXCEPT(Exception::ERR_FILE_NOT_FOUND,
                "Cannot locate resource " + name + " in resource group " + group
                    + ".",
                "SoundManager::playMusic");
  }
  wavResPtr->load();
  auto &src{wavResPtr->_getAudioSource()};

  return SoundHandle{mSoloud->playBackground(src, volume)};
}

float SoundManager::_getVolume(const SoundHandle &sound) const {
  return mSoloud->getVolume(sound.mHandle);
}

void SoundManager::_setVolume(SoundHandle &sound, float volume) const {
  mSoloud->setVolume(sound.mHandle, volume);
}

void SoundManager::_stop(SoundHandle &sound) const {
  mSoloud->stop(sound.mHandle);
}

SoundManager &SoundHandle::mgr() const {
  return SoundManager::getSingleton();
}

float SoundHandle::getVolume() const {
  return mgr()._getVolume(*this);
}

void SoundHandle::setVolume(float volume) {
  mgr()._setVolume(*this, volume);
}

void SoundHandle::stop() {
  mgr()._stop(*this);
}

} // namespace Ogre