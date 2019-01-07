#include "fnt_loader.hpp"
#include "fs/path.hpp"
#include "settings.hpp"
#include <OgreDataStream.h>
#include <OgreFont.h>
#include <OgreImage.h>
#include <OgreMaterialManager.h>
#include <OgrePixelFormat.h>
#include <OgreResourceGroupManager.h>
#include <OgreTextureManager.h>
#include <spdlog/spdlog.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace oo {

void FntLoader::loadResource(Ogre::Resource *resource) {
  auto *font{dynamic_cast<Ogre::Font *>(resource)};
  // TODO: Handle this properly
  assert(font != nullptr);

  FntLoaderImpl impl(font);

  const std::string name{font->getName()};
  const std::string group{font->getGroup()};

  // Create the new texture
  const std::string texName{name + "Texture"};
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  auto texPtr{texMgr.create(texName, group, true, &impl)};
  texPtr->setTextureType(Ogre::TEX_TYPE_2D);
  texPtr->setNumMipmaps(0);
  texPtr->load();

  // Set the font properties
  font->setType(Ogre::FontType::FT_IMAGE);
  font->setSource(texName);

  // Ideally we'd just call font->loadImpl() now, but we can't so this just
  // reimplements the specific bits we need.
  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  auto matPtr{matMgr.create(name + std::string{"Material"}, group)};

  auto *pass{matPtr->getTechnique(0)->getPass(0)};
  pass->setVertexColourTracking(Ogre::TVC_DIFFUSE);

  auto *texUnitState{pass->createTextureUnitState(texName)};
//  texUnitState->setTexture(newTexPtr);
  texUnitState->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
  texUnitState->setTextureFiltering(Ogre::FO_LINEAR, Ogre::FO_LINEAR,
                                    Ogre::FO_NONE);

  matPtr->setSceneBlending(texPtr->hasAlpha() ? Ogre::SBT_TRANSPARENT_ALPHA
                                              : Ogre::SBT_ADD);

//  font->_setTexture(newTexPtr);
  font->_setMaterial(matPtr);
}

void FntLoaderImpl::loadResource(Ogre::Resource *resource) {
  auto *tex{dynamic_cast<Ogre::Texture *>(resource)};
  assert(tex != nullptr);

  const std::string name{mFont->getName()};
  const std::string group{mFont->getGroup()};

  // Open data stream to the font file
  auto &resGrpMgr{Ogre::ResourceGroupManager::getSingleton()};
  auto fntPtr{resGrpMgr.openResource(name, group)};

  // Skip the first bit of header
  fntPtr->skip(12u);

  // Read the name of the corresponding bitmap and skip the rest of the header
  std::string srcBasename{};
  char c{0};
  while (fntPtr->read(&c, 1) == 1 && c != 0) srcBasename.push_back(c);
  fntPtr->skip(5u * 56u - (srcBasename.size() + 1u) + 4u);
  srcBasename += ".tex";

  // Build the path to the bitmap file and load it
  oo::Path srcFolder{std::string{oo::Path{name}.folder()}};
  oo::Path srcPath{std::move(srcFolder) / oo::Path{std::move(srcBasename)}};
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  auto srcPtr{texMgr.load(srcPath.c_str(), group)};

  uint32_t srcWidth{srcPtr->getWidth()};
  uint32_t srcHeight{srcPtr->getHeight()};

  spdlog::get(oo::LOG)->info("Src: w = {}, h = {}", srcWidth, srcHeight);

  // Glyphs are stored encoded with Windows-1252 and stored in order, whereas
  // Ogre wants Unicode codepoints.
  constexpr std::array<Ogre::Font::CodePoint, 256> encoding{
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

  // When rendering text, the characters are scaled to all be the same height,
  // as it is expected that each glyph have space above and below to
  // compensate for the ascenders and descenders of the other glyphs.
  // We therefore need to find the largest bounding box height and create a new
  // texture for the glyphs with space inserted above and below each glyph as
  // appropriate.

  float maxGlyphHeight{0.0f};
  float maxGlyphWidth{0.0f};
  uint32_t ascentOfTallestGlyph{0};

  std::array<Glyph, 256> glyphs{};

  for (std::size_t i = 0; i < 256; ++i) {
    Glyph &g{glyphs[i]};
    fntPtr->read(reinterpret_cast<char *>(&g), sizeof(Glyph));

    const float width{g.topRight.x - g.topLeft.x};
    const float height{g.bottomLeft.y - g.topLeft.y};
    if (width > maxGlyphWidth) maxGlyphWidth = width;
    if (height > maxGlyphHeight) {
      maxGlyphHeight = height;
      ascentOfTallestGlyph = static_cast<uint32_t>(g.ascent);
    }
  }

  // Convert to pixels from normalized coordinates
  maxGlyphHeight *= srcHeight;
  maxGlyphWidth *= srcWidth;

  spdlog::get(oo::LOG)->info("Largest width = {}", maxGlyphWidth);
  spdlog::get(oo::LOG)->info("Largest height = {}", maxGlyphHeight);
  spdlog::get(oo::LOG)->info("Maximum ascent = {}", ascentOfTallestGlyph);

  // Compute new texture size
  const float dstWidthFloat = [&]() {
    const float glyphLongestSide{std::max(maxGlyphHeight, maxGlyphWidth)};
    const float dstArea{(maxGlyphHeight + mFont->getCharacterSpacer())
                            * (maxGlyphWidth + mFont->getCharacterSpacer())
                            * 256.0f};
    const float dstWidthRaw{std::sqrt(dstArea) + glyphLongestSide};
    return std::exp2(std::ceil(std::log2(dstWidthRaw)));
  }();

  if (dstWidthFloat > std::numeric_limits<uint32_t>::max()) {
    throw std::runtime_error("Cannot create font texture, too large");
  }

  // Allocate data for the new texture
  const auto dstWidth{static_cast<uint32_t>(dstWidthFloat)};
  const auto dstHeight{dstWidth};
  std::size_t numBytes{dstWidth * dstWidth * 4u};
  auto dstStreamPtr{std::make_shared<Ogre::MemoryDataStream>(numBytes)};
  Ogre::PixelBox dstBox(dstWidth, dstWidth, 1u, Ogre::PF_BYTE_RGBA,
                        dstStreamPtr->getPtr());
  // TODO: A blit function that copies a single value over an entire PixelBox
  std::memset(dstStreamPtr->getPtr(), 0, numBytes);

  spdlog::get(oo::LOG)->info("dst w = {}", dstWidth);

  // Convert source texture to an image
  Ogre::Image srcImg{};
  srcPtr->convertToImage(srcImg);
  Ogre::PixelBox srcBox{srcImg.getPixelBox()};
  srcImg.save("srcimg.dds");

  // Copy each glyph from the old image to the new image, inserting padding
  // using the ascender height.
  uint32_t left{0};
  uint32_t top{0};
  for (std::size_t i = 0; i < 256; ++i) {
    const auto &g{glyphs[i]};

    const Ogre::Box srcBounds{
        static_cast<uint32_t>(g.topLeft.x * srcWidth),
        static_cast<uint32_t>(g.topLeft.y * srcHeight),
        static_cast<uint32_t>(g.bottomRight.x * srcWidth),
        static_cast<uint32_t>(g.bottomRight.y * srcHeight)
    };
    Ogre::PixelBox src{srcBox.getSubVolume(srcBounds)};

    // Picture a box that is width * maxGlyphHeight. The baseline is placed
    // a distance ascentOfTallestGlyph from the top of the box. The current
    // glyph is placed within the box such that the distance from the top of the
    // glyph to the baseline is the glyph's ascent.
    const auto width
        {static_cast<uint32_t>(srcWidth * (g.topRight.x - g.topLeft.x))};
    const auto height
        {static_cast<uint32_t>(srcHeight * (g.bottomLeft.y - g.topLeft.y))};
    const auto ascent{static_cast<uint32_t>(g.ascent)};
    const auto glyphTop{top + ascentOfTallestGlyph - ascent};

    spdlog::get(oo::LOG)->info("{}: w = {}, h = {}, a = {}",
                               i, width, height, ascent);

    const Ogre::Box dstBounds{left, glyphTop, left + width, glyphTop + height};
    Ogre::PixelBox dst{dstBox.getSubVolume(dstBounds)};

    spdlog::get(oo::LOG)->info("\t[{}, {}, {}, {}] -> [{}, {}, {}, {}]",
                               srcBounds.left,
                               srcBounds.top,
                               srcBounds.right,
                               srcBounds.bottom,
                               dstBounds.left,
                               dstBounds.top,
                               dstBounds.right,
                               dstBounds.bottom);

    // Copy the glyph over and update the font to point to it.
    Ogre::PixelUtil::bulkPixelConversion(src, dst);
    mFont->setGlyphTexCoords(encoding[i],
                             static_cast<float>(left) / dstWidth,
                             static_cast<float>(top) / dstHeight,
                             static_cast<float>(left + width) / dstWidth,
                             static_cast<float>(top + maxGlyphHeight)
                                 / dstHeight,
                             1.0f);
    mFont->setGlyphAspectRatio(encoding[i],
                               static_cast<float>(width) / maxGlyphHeight);

    left += width + mFont->getCharacterSpacer();
    if (left + maxGlyphWidth >= dstWidth) {
      left = 0;
      top += maxGlyphHeight + mFont->getCharacterSpacer();
    }
  }

  Ogre::ConstImagePtrList imgPtrs{};
  Ogre::Image dstImg{};
  dstImg.loadRawData(dstStreamPtr,
                     dstBox.getWidth(), dstBox.getHeight(),
                     dstBox.getDepth(), dstBox.format);
  dstImg.save("dstimg.dds");
  imgPtrs.push_back(&dstImg);
  tex->_loadImages(imgPtrs);
}

} // namespace oo
