#include "ogresoloud/sound_manager.hpp"
#include "ogresoloud/wav_resource.hpp"
#include <Ogre.h>

namespace Ogre {

WavResource::WavResource(Ogre::ResourceManager *creator,
                         const Ogre::String &name,
                         Ogre::ResourceHandle handle,
                         const Ogre::String &group,
                         bool isManual,
                         Ogre::ManualResourceLoader *loader)
    : Resource(creator, name, handle, group, isManual, loader) {}

bool WavResource::getLoopingEnabled() const noexcept {
  return mIsLooping;
}

float WavResource::getVolume() const noexcept {
  return mWav.mVolume;
}

void WavResource::setLoopingEnabled(bool looping) noexcept {
  mIsLooping = looping;
  mWav.setLooping(looping);
}

void WavResource::setVolume(float volume) noexcept {
  mWav.setVolume(volume);
}

SoLoud::AudioSource &WavResource::_getAudioSource() noexcept {
  return mWav;
}

void WavResource::loadImpl() {
  auto &resGrpMgr{ResourceGroupManager::getSingleton()};

  // Read the file into a buffer and pass it to SoLoud without transferring
  // ownership.
  auto dataStreamPtr{resGrpMgr.openResource(mName, mGroup)};
  mWavData.resize(dataStreamPtr->size());
  dataStreamPtr->read(mWavData.data(), mWavData.size());

  if (mWavData.size() > std::numeric_limits<unsigned int>::max()) {
    OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR,
                "Wav file is larger than the size supported by SoLoud",
                "WavResource::loadImpl()");
  }
  const auto wavSize{static_cast<unsigned int>(mWavData.size())};

  if (auto err{static_cast<SoLoud::SOLOUD_ERRORS>(
                   mWav.loadMem(mWavData.data(), wavSize, false, false))};
      err) {
    OGRE_EXCEPT(SoundManager::_errorToExceptionCode(err),
                String{"Failed to load Wav resource: "}
                    + SoundManager::_errorToString(err),
                "WavResource::loadImpl()");
  }
}

void WavResource::unloadImpl() {}

} // namespace Ogre