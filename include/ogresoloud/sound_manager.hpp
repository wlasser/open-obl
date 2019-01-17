#ifndef OPENOBLIVION_OGRESOLOUD_SOUND_MANAGER_HPP
#define OPENOBLIVION_OGRESOLOUD_SOUND_MANAGER_HPP

#include <soloud/soloud.h>
#include <OgreSingleton.h>
#include <OgreResourceGroupManager.h>
#include <memory>

namespace Ogre {

class SoundHandle;

class SoundManager : public Singleton<SoundManager> {
 private:
  std::unique_ptr<SoLoud::Soloud> mSoloud{};

  /// Convert a SoLoud error into a string representation.
  /// This is needed in addition to errorToExceptionCode as there is not a
  /// one-to-one mapping of SoLoud error into Ogre exception codes.
  static String errorToString(SoLoud::SOLOUD_ERRORS error);

  /// Convert a SoLoud error into an Ogre::Exception code.
  static Exception::ExceptionCodes
  errorToExceptionCode(SoLoud::SOLOUD_ERRORS error) noexcept;

 public:
  explicit SoundManager();
  ~SoundManager() = default;

  static SoundManager &getSingleton();
  static SoundManager *getSingletonPtr();

  /// Play an Ogre::WavResource as background music.
  /// The music is played at full volume by default.
  SoundHandle playMusic(const String &name,
                        const String &group,
                        float volume = -1.0f) const;

  /// Internal method to get the volume setting of a sound.
  float _getVolume(const SoundHandle &sound) const;
  /// Internal method to set the volume setting of a sound.
  void _setVolume(SoundHandle &sound, float volume) const;
  /// Internal method to stop a playing sound.
  void _stop(SoundHandle &sound) const;
};

class SoundHandle {
 private:
  friend class SoundManager;

  SoLoud::handle mHandle{};

  SoundManager &mgr() const;

  explicit SoundHandle(SoLoud::handle handle) : mHandle(handle) {}

 public:
  /// Get the volume setting of this sound.
  float getVolume() const;

  /// Set the volume setting of this sound.
  /// \param volume a value between `0.0f` and `1.0f`, with `1.0f` being the
  ///               volume of the source sound and `0.0f` being silent.
  void setVolume(float volume);

  /// Stop playing this sound.
  void stop();
};

} // namespace Ogre

#endif // OPENOBLIVION_SOLOUD_SOUND_MANAGER_HPP
