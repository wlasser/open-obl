#include "audio.hpp"
#include "fs/path.hpp"
#include "game_settings.hpp"
#include "util/settings.hpp"
#include <spdlog/spdlog.h>

namespace oo {

float MusicManager::getMusicVolume() const noexcept {
  return mVolume;
}

void MusicManager::setMusicVolume(float volume) noexcept {
  mVolume = volume;
  if (mSoundHandle) mSoundHandle->setVolume(volume);
}

bool MusicManager::isPlayingMusic() const noexcept {
  return mSoundHandle.has_value();
}

MusicType MusicManager::getCurrentType() const noexcept {
  return mCurrentType;
}

MusicType MusicManager::getNextType() const noexcept {
  return mNextType;
}

void MusicManager::setMusicType(MusicType type, bool force) noexcept {
  mNextType = type;
  if (mSoundHandle && !force) return;
  else if (mSoundHandle && force) {
    if (type == mCurrentType) return;
    mSoundHandle->stop();
    mSoundHandle.reset();
  }

  mCurrentType = type;
  if (type != MusicType::None) {
    try {
      auto &sndMgr{Ogre::SoundManager::getSingleton()};
      mSoundHandle = sndMgr.playMusic(selectTrack(type), oo::RESOURCE_GROUP,
                                      mVolume);
    } catch (const std::exception &e) {
      spdlog::get(oo::LOG)->error("Failed to play music of type {}: {}",
                                  static_cast<uint32_t>(type),
                                  e.what());
    }
  }
}

void MusicManager::setMusicType(record::SNAM_WRLD type, bool force) noexcept {
  using Type = record::raw::SNAM_WRLD;
  switch (type.data) {
    case Type::Default: return setMusicType(MusicType::Default, force);
    case Type::Dungeon: return setMusicType(MusicType::Dungeon, force);
    case Type::Public: return setMusicType(MusicType::Public, force);
  }
}

void MusicManager::setMusicType(record::XCMT type, bool force) noexcept {
  using Type = record::raw::XCMT;
  switch (type.data) {
    case Type::Default: return setMusicType(MusicType::Default, force);
    case Type::Dungeon: return setMusicType(MusicType::Dungeon, force);
    case Type::Public: return setMusicType(MusicType::Public, force);
  }
}

void MusicManager::stopMusic() noexcept {
  if (mSoundHandle) mSoundHandle->stop();
}

std::optional<Ogre::SoundHandle> MusicManager::playMusic() noexcept {
  setMusicType(getNextType(), false);
  return mSoundHandle;
}

std::optional<Ogre::SoundHandle> MusicManager::playDeathMusic() noexcept {
  playSpecial("music/special/death.mp3");
  return mSoundHandle;
}

std::optional<Ogre::SoundHandle> MusicManager::playSuccessMusic() noexcept {
  playSpecial("music/special/success.mp3");
  return mSoundHandle;
}

std::optional<Ogre::SoundHandle> MusicManager::playTitleMusic() noexcept {
  const auto &gameSettings{oo::GameSettings::getSingleton()};

  oo::Path musicPath{"music"};
  const oo::Path musicFilename{gameSettings.get("General.sMainMenuMusic",
                                                "special/tes4title.mp3")};
  musicPath /= musicFilename;
  playSpecial(musicPath.c_str());
  setMusicVolume(gameSettings.get("Audio.fMainMenuMusicVolume", 1.0f));

  return mSoundHandle;
}

void MusicManager::addTrack(MusicType type, std::string filename) {
  const auto index{static_cast<uint32_t>(type)};
  if (index >= NUM_TYPES) {
    throw std::runtime_error("Invalid music type");
  }

  mTracks[index].emplace_back(std::move(filename));
}

void MusicManager::update(float delta) {
  if (!mSoundHandle) return;

  mCurrentTime += delta;
  if (mCurrentTime > mSoundHandle->getLength()) {
    mCurrentTime = 0.0f;
    mSoundHandle->stop();
    mSoundHandle.reset();
    playMusic();
  }
}

std::string MusicManager::selectTrack(oo::MusicType type) {
  if (static_cast<uint32_t>(type) >= NUM_TYPES) {
    throw std::runtime_error("Invalid music type");
  }

  const auto &tracks{mTracks[static_cast<uint32_t>(type)]};
  if (tracks.empty()) {
    throw std::runtime_error("No tracks of given music type");
  }

  std::string track;
  std::sample(tracks.begin(), tracks.end(), &track, 1u, mGen);

  return track;
}

void MusicManager::playSpecial(const std::string &filename) {
  if (mSoundHandle) {
    mSoundHandle->stop();
    mSoundHandle.reset();
  }
  mCurrentType = MusicType::Special;

  try {
    auto &sndMgr{Ogre::SoundManager::getSingleton()};
    mSoundHandle = sndMgr.playMusic(filename, oo::RESOURCE_GROUP, mVolume);
  } catch (const std::exception &e) {
    spdlog::get(oo::LOG)->error("Failed to play special music {}: {}",
                                filename,
                                e.what());
  }
}

} // namespace oo