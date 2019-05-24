#ifndef OPENOBL_OGRE_SOLOUD_WAV_RESOURCE_HPP
#define OPENOBL_OGRE_SOLOUD_WAV_RESOURCE_HPP

#include <OgreResource.h>
#include <soloud/soloud_wav.h>
#include <vector>

namespace Ogre {

class WavResource : public Ogre::Resource {
 public:
  WavResource(ResourceManager *creator,
              const String &name,
              ResourceHandle handle,
              const String &group,
              bool isManual = false,
              ManualResourceLoader *loader = nullptr);

  ~WavResource() override {
    unload();
  }

  /// Whether the sound will loop.
  bool getLoopingEnabled() const noexcept;

  /// The default volume for all new instances of this sound.
  float getVolume() const noexcept;

  /// The length of the sound, in seconds.
  float getLength() noexcept;

  /// Set whether the sound will loop.
  void setLoopingEnabled(bool looping) noexcept;

  /// Set the default volume for all new instances of this sound.
  void setVolume(float volume) noexcept;

  /// Internal method to get the underlying SoLoud audio source.
  SoLoud::AudioSource &_getAudioSource() noexcept;

 protected:
  void loadImpl() override;
  void unloadImpl() override;

 private:
  /// Buffer for mWav data; we retain ownership.
  std::vector<unsigned char> mWavData{};

  /// The actual audio source.
  SoLoud::Wav mWav{};

  /// Whether the sound should loop.
  bool mIsLooping;
};

using WavResourcePtr = std::shared_ptr<WavResource>;

} // namespace Ogre

#endif // OPENOBL_OGRE_SOLOUD_WAV_RESOURCE_HPP
