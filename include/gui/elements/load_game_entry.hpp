#ifndef OPENOBL_GUI_LOAD_GAME_ENTRY_HPP
#define OPENOBL_GUI_LOAD_GAME_ENTRY_HPP

#include "gui/elements/rect.hpp"
#include <array>

namespace gui {

class LoadGameEntry : public Rect {
 private:
  /// `user0`: Index of this entry in its parent list.
  float mListIndex{};
  /// `user1`: Name of the image file to display.
  std::string mImageFilename{};
  /// `user2`: Name of the savegame.
  std::string mSaveName{};
  /// `user3`: FormId of the savegame.
  std::string mSaveFormId{};
  /// `user4`--`user9` are all unused.
  std::array<float, 6> mUnused{};
  /// `user10`: Unknown.
  bool mUser10{};

  UserTraitInterface<float, std::string, std::string, std::string,
                     float, float, float, float, float, float,
                     bool> mInterface{std::make_tuple(
      &mListIndex, &mImageFilename, &mSaveName, &mSaveFormId,
      &mUnused[0], &mUnused[1], &mUnused[2], &mUnused[3], &mUnused[4],
      &mUnused[5], &mUser10)};

 public:
  explicit LoadGameEntry(std::string name) : Rect(std::move(name)) {}

  auto getUserOutputTraitInterface() const {
    return std::make_tuple(&mListIndex, &mImageFilename, &mSaveName,
                           &mSaveFormId, &mUnused[0], &mUnused[1],
                           &mUnused[2], &mUnused[3], &mUnused[4],
                           &mUnused[5], &mUser10);
  }

  BUILD_USER_TRAIT_INTERFACE(mInterface);
};

} // namespace gui

#endif // OPENOBL_GUI_LOAD_GAME_ENTRY_HPP
