#ifndef OPENOBLIVION_FNT_LOADER_HPP
#define OPENOBLIVION_FNT_LOADER_HPP

#include <Ogre.h>
#include <OgreFont.h>

namespace oo {

class FntLoader : public Ogre::ManualResourceLoader {
 public:
  void loadResource(Ogre::Resource *resource) override;
};

class FntLoaderImpl : public Ogre::ManualResourceLoader {
 private:
  Ogre::Font *mFont;

  struct Glyph {
    /// Unknown, always zero.
    float unknown1{};

    /// Coordinates of the bounding box of the glyph in the bitmap, given in
    /// normalized coordinates.
    /// @{
    Ogre::Vector2 topLeft{};
    Ogre::Vector2 topRight{};
    Ogre::Vector2 bottomLeft{};
    Ogre::Vector2 bottomRight{};
    /// @}

    /// Dimensions of the bounding box of the displayed glyph, in pixels.
    /// Possibly different to the dimensions of the bounding box in the bitmap.
    /// @{
    float width{};
    float height{};
    /// @}

    /// Unknown, always zero.
    float unknown2{};

    /// Additional offset to display from the end of this character to the start
    /// of the next. Used for glyphs without any representation in the bitmap,
    /// such as space.
    float additionalOffset{};

    /// Distance from the top of the glyph to the baseline.
    float ascent{};
  };

 public:
  FntLoaderImpl(Ogre::Font *font) : mFont(font) {}

  void loadResource(Ogre::Resource *resource) override;
};

} // namespace oo

#endif // OPENOBLIVION_FNT_LOADER_HPP
