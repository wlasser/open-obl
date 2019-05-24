#ifndef OPENOBL_FNT_LOADER_HPP
#define OPENOBL_FNT_LOADER_HPP

#include <OgreFont.h>
#include <OgreImage.h>
#include <OgreResource.h>
#include <OgreVector.h>
#include <array>

namespace oo {

class Path;

/// Ogre::ManualResourceLoader for `fnt`/`tex` bitmap font file pairs.
///
/// A `fnt` file contains the name of a corresponding `tex` image file---which
/// is the physical bitmap source of the font---and the bounding boxes of each
/// glyph within the `tex` file. This loader expects the `fnt` file; it will
/// load the `tex` file and add all the glyphs automatically.
///
/// This is slightly more awkward than it sounds, as the bounding boxes for each
/// glyph are usually minimal, but the engine expects there to be space above
/// and below to make each glyph the same height. This extra space compensates
/// for the ascenders and descenders of the other glyphs in the font, and makes
/// it much easier align to place each glyph when rendering text. In particular,
/// since each Ogre::GlyphInfo stores only the bounding box and the aspect
/// ratio, there is no baseline information to align to. Instead, every glyph
/// is scaled to the font height and placed with aligned top-left corners.
///
/// To support this, we need to bake in the baseline position to each glyph
/// by constructing a new texture with uniform-height bounding boxes for each
/// glyph. Since this texture is not backed directly by a file, we need to use
/// another Ogre::ManualResourceLoader, namely oo::FntLoaderImpl. Unfortunately
/// this new loader needs to know about which Ogre::Font it is creating an
/// Ogr::Texture for, so a new loader must be created on each Ogre::Font load.
/// Since these loaders are temporary, this obviously causes issues if the
/// new texture is reloaded. It is therefore imperative that fonts created using
/// the oo::FntLoader are never unloaded.
///
/// \warning Do not reload fonts created using this loader, once loaded they
///          should stay loaded until the end of the program.
/// \todo Find a solution to the texture reload problem, that isn't just
///       "Don't unload your fonts".
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

  /// Map from Windows-1252 encoding to Unicode codepoints.
  constexpr static std::array<Ogre::Font::CodePoint, 256> WIN_1252{
      0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
      0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
      0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
      0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
      ' ', '!', '"', '#', '$', '%', '&', '\'',
      '(', ')', '*', '+', ',', '-', '.', '/',
      '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', ':', ';', '<', '=', '>', '?',
      '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
      'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
      'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
      'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
      '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
      'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
      'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
      'x', 'y', 'z', '{', '|', '}', '~', 0x007F,
      0x20AC, 0x0000, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
      0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x0000, 0x017D, 0x0000,
      0x0000, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
      0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x0000, 0x017E, 0x0178,
      0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
      0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
      0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
      0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
      0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
      0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
      0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
      0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
      0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
      0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
      0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
      0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF
  };

  /// Skip past the header, returning the filename of the bitmap tex file.
  oo::Path readHeader(const Ogre::DataStreamPtr &fntPtr) const;

  /// Put the tex file basename into the same folder as `mFont`.
  oo::Path resolveTexName(const oo::Path &basename) const;

  /// Load the tex file as an image.
  Ogre::Image getTexImage(const oo::Path &texPath) const;

 public:
  FntLoaderImpl(Ogre::Font *font) : mFont(font) {}

  void loadResource(Ogre::Resource *resource) override;
};

} // namespace oo

#endif // OPENOBL_FNT_LOADER_HPP
