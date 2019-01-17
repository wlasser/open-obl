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

String SoundManager::_errorToString(SoLoud::SOLOUD_ERRORS error) {
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
SoundManager::_errorToExceptionCode(SoLoud::SOLOUD_ERRORS error) noexcept {
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
    OGRE_EXCEPT(_errorToExceptionCode(err),
                String{"Failed to initialise SoLoud: "} + _errorToString(err),
                "SoundManager::SoundManager()");
  }

  mMasterBus = std::make_unique<MixingBus>();
  mMasterBus->second = SoundHandle{mSoloud->play(mMasterBus->first)};

  mMusicBus = std::make_unique<MixingBus>();
  mMusicBus->second = SoundHandle{mMasterBus->first.play(mMusicBus->first)};

  mSoloud->setVolume(mMasterBus->second->mHandle, 1.0f);
  mSoloud->setVolume(mMusicBus->second->mHandle, 1.0f);
}

SoundHandle SoundManager::playMusic(const String &name,
                                    const String &group,
                                    float volume) {
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

  SoundHandle handle{mMusicBus->first.play(src, volume)};
  mSoloud->setPanAbsolute(handle.mHandle, 1.0f, 1.0f);

  return handle;
}

SoundHandle SoundManager::playSound(const String &name, const String &group,
                                    const String &busName, float volume) {
  auto it{mMixingBuses.find(busName)};
  if (it == mMixingBuses.end()) {
    OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
                "Cannot find a MixingBus named " + busName + ".",
                "SoundManager::playSound");
  }
  auto &bus{it->second};

  auto &waveResMgr{WavResourceManager::getSingleton()};

  auto wavResPtr{waveResMgr.getByName(name, group)};
  if (!wavResPtr) {
    OGRE_EXCEPT(Exception::ERR_FILE_NOT_FOUND,
                "Cannot locate resource " + name + " in resource group " + group
                    + ".",
                "SoundManager::playSound");
  }
  wavResPtr->load();
  auto &src{wavResPtr->_getAudioSource()};

  return SoundHandle{bus.first.play(src, volume)};
}

SoundHandle SoundManager::createMixingBus(const String &name) {
  auto[it, emplaced]{mMixingBuses.try_emplace(name)};
  if (!emplaced) {
    OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM,
                "MixingBus with the name " + name + " already exists.",
                "SoundManager::createMixingBus");
  }
  auto &bus{it->second};
  bus.second = SoundHandle{mMasterBus->first.play(bus.first)};
  return *bus.second;
}

SoundHandle SoundManager::getMixingBus(const String &name) const {
  auto it{mMixingBuses.find(name)};
  if (it == mMixingBuses.end()) {
    OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
                "Cannot find a MixingBus named " + name + ".",
                "SoundManager::getMixingBus");
  }
  return *it->second.second;
}

SoundHandle SoundManager::getMusicBus() const {
  return *mMusicBus->second;
}

SoundHandle SoundManager::getMasterBus() const {
  return *mMasterBus->second;
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