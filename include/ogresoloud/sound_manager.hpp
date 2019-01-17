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

  using MixingBus = std::pair<SoLoud::Bus, std::optional<SoundHandle>>;
  using MixingBusMap = std::map<String, MixingBus>;

  /// Master mixing bus.
  std::unique_ptr<MixingBus> mMasterBus{};
  /// Music mixing bus. This is a child of the master mixing bus.
  std::unique_ptr<MixingBus> mMusicBus{};
  /// Child mixing buses of the master mixing bus.
  MixingBusMap mMixingBuses{};

 public:

  explicit SoundManager();
  ~SoundManager() = default;

  static SoundManager &getSingleton();
  static SoundManager *getSingletonPtr();

  /// Play an Ogre::WavResource as background music through the music mixing
  /// bus. The music is played at full volume by default.
  SoundHandle playMusic(const String &name,
                        const String &group,
                        float volume = -1.0f);

  /// Play an Ogre::WavResource through the given mixing bus. Throws an
  /// exception if no such mixing bus exists.
  SoundHandle playSound(const String &name, const String &group,
                        const String &busName, float volume = -1.0f);

  /// Create a new named mixing bus.
  SoundHandle createMixingBus(const String &name);

  /// Return a handle to the named mixing bus. Throws an exception if no such
  /// mixing bus exists.
  SoundHandle getMixingBus(const String &name) const;

  /// Return a handle to the music mixing bus.
  SoundHandle getMusicBus() const;

  /// Return a handle to the master mixing bus.
  SoundHandle getMasterBus() const;

  /// Internal method to get the volume setting of a sound.
  float _getVolume(const SoundHandle &sound) const;
  /// Internal method to set the volume setting of a sound.
  void _setVolume(SoundHandle &sound, float volume) const;
  /// Internal method to stop a playing sound.
  void _stop(SoundHandle &sound) const;

  /// Convert a SoLoud error into a string representation.
  /// This is needed in addition to errorToExceptionCode as there is not a
  /// one-to-one mapping of SoLoud error into Ogre exception codes.
  static String _errorToString(SoLoud::SOLOUD_ERRORS error);

  /// Convert a SoLoud error into an Ogre::Exception code.
  static Exception::ExceptionCodes
  _errorToExceptionCode(SoLoud::SOLOUD_ERRORS error) noexcept;
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
