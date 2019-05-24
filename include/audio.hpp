#ifndef OPENOBL_AUDIO_HPP
#define OPENOBL_AUDIO_HPP

#include "ogresoloud/sound_manager.hpp"
#include "record/subrecords.hpp"
#include <random>

namespace oo {

enum class MusicType : uint32_t {
  Default = 0u,
  Public = 1u,
  Dungeon = 2u,
  Battle,
  N,
  Special,
  None = 0xffffffff
};

/// Wrapper around `Ogre::SoundManager` specifically for playing music.
class MusicManager {
 public:
  /// Return the volume of the currently playing music, if any, or the volume
  /// that any subsequent music will play at otherwise.
  /// Notably, if no music is playing then this will not necessarily return `0`.
  float getMusicVolume() const noexcept;

  /// Set the volume of the currently playing music, if any, or the volume
  /// that any subsequent music will play at otherwise.
  void setMusicVolume(float volume) noexcept;

  /// Return whether music is actually playing currently.
  bool isPlayingMusic() const noexcept;

  /// Return the type of the currently playing music.
  MusicType getCurrentType() const noexcept
  /*C++20: [[requires : isPlayingMusic()]]*/;

  /// Return the type of music that will be played next.
  MusicType getNextType() const noexcept;

  /// Set the type of music to be played next.
  /// If no music is playing, or `force` is set and `type` does not equal the
  /// currently playing music type, then the current track (if any) will stop
  /// immediately and a track of the new `type` will begin. Otherwise, the
  /// music type will not change until the end of the currently playing track.
  void setMusicType(MusicType type, bool force = false) noexcept;

  /// \overload setMusicType(MusicType, bool)
  void setMusicType(record::SNAM_WRLD type, bool force = false) noexcept;

  /// \overload setMusicType(MusicType, bool)
  void setMusicType(record::XCMT type, bool force = false) noexcept;

  /// Stop the currently playing music, if any.
  void stopMusic() noexcept;

  /// If no music is playing, select a track of the next type and play it.
  /// Equivalent to `setMusicType(getNextType(), false)`.
  /// Returns a handle to the currently playing music after setting the music
  /// type. If no music was playing but no new music could be played for some
  /// reason, then an empty optional is returned.
  std::optional<Ogre::SoundHandle> playMusic() noexcept;

  /// Special Music
  /// These functions play specific pieces of music tied to certain in-game
  /// events. They use a fixed track name instead of a sampling from a
  /// selection, and force the music to change immediately.
  /// @{
  std::optional<Ogre::SoundHandle> playDeathMusic() noexcept;
  std::optional<Ogre::SoundHandle> playSuccessMusic() noexcept;
  std::optional<Ogre::SoundHandle> playTitleMusic() noexcept;
  /// @}

  /// Mark an `Ogre::WavResource` as being part of the given music `type`,
  /// making it available for playing.
  /// \throws std::runtime_error if `type` does not actually name a music type
  ///                            e.g. if `type` is `N` or `None`.
  void addTrack(MusicType type, std::string filename);

  /// Update the internal clock, playing new music when necessary.
  void update(float delta);

 private:
  /// Random device for track selection.
  std::random_device mRd{};

  /// Random generator for track selection.
  std::mt19937 mGen{mRd()};

  /// Number of different music types.
  constexpr static std::size_t
      NUM_TYPES = static_cast<std::size_t>(MusicType::N);

  /// Track filenames for each music type.
  std::array<std::vector<std::string>, NUM_TYPES> mTracks{};

  /// Handle to currently playing music, if any.
  std::optional<Ogre::SoundHandle> mSoundHandle{};

  /// Type of music currently playing. Meaningless if no music is playing.
  MusicType mCurrentType{MusicType::None};

  /// Type of music to be played next.
  MusicType mNextType{MusicType::None};

  /// Volume of music currently playing, if any, or the volume that the music
  /// will be played at next.
  float mVolume{-1.0f};

  /// Duration the current track has been playing for.
  float mCurrentTime{0.0f};

  /// Randomly sample a track of the given music type.
  /// \throws std::runtime_error if `type` does not actually name a music type
  ///                            e.g. if `type` is `N` or `None`.
  /// \throws std::runtime_error if there are no tracks of the given `type`.
  std::string selectTrack(MusicType type);

  void playSpecial(const std::string &filename);
};

} // namespace oo

#endif // OPENOBL_AUDIO_HPP
