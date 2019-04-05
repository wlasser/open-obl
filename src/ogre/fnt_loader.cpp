#include "ogre/fnt_loader.hpp"
#include "fs/path.hpp"
#include <OgreDataStream.h>
#include <OgreFont.h>
#include <OgreImage.h>
#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgrePixelFormat.h>
#include <OgreResourceGroupManager.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
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
  const std::string texName{name + "Texture"};

  // Create the new texture
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
  auto matPtr{matMgr.create(name + "Material", group)};
  matPtr->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);

  auto *pass{matPtr->getTechnique(0)->getPass(0)};
  pass->setVertexColourTracking(Ogre::TVC_DIFFUSE);

  auto *texUnit{pass->createTextureUnitState(texName)};
  texUnit->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
  texUnit->setTextureFiltering(Ogre::FO_LINEAR, Ogre::FO_LINEAR, Ogre::FO_NONE);

  font->_setMaterial(matPtr);
}

oo::Path FntLoaderImpl::readHeader(const Ogre::DataStreamPtr &fntPtr) const {
  // Skip the first bit of header
  fntPtr->skip(12u);

  // Read the name of the corresponding bitmap and skip the rest of the header
  std::string srcBasename{};
  char c{0};
  while (fntPtr->read(&c, 1) == 1 && c != 0) srcBasename.push_back(c);
  fntPtr->skip(5u * 56u - (srcBasename.size() + 1u) + 4u);
  srcBasename += ".tex";

  return oo::Path(std::move(srcBasename));
}

oo::Path FntLoaderImpl::resolveTexName(const oo::Path &basename) const {
  // TODO: I think oo::Path needs a std::string_view constructor
  oo::Path folder{std::string{oo::Path{mFont->getName()}.folder()}};
  return folder / basename;
}

Ogre::Image FntLoaderImpl::getTexImage(const oo::Path &texPath) const {
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  auto texPtr{texMgr.load(texPath.c_str(), mFont->getGroup())};

  Ogre::Image texImg{};
  texPtr->convertToImage(texImg);

  return texImg;
}

void FntLoaderImpl::loadResource(Ogre::Resource *resource) {
  auto *tex{dynamic_cast<Ogre::Texture *>(resource)};
  assert(tex != nullptr);

  // Open data stream to the font file
  auto &resGrpMgr{Ogre::ResourceGroupManager::getSingleton()};
  auto fntPtr{resGrpMgr.openResource(mFont->getName(), mFont->getGroup())};

  // Build the path to the bitmap file and load it
  const oo::Path srcBasename{readHeader(fntPtr)};
  const oo::Path srcPath{resolveTexName(srcBasename)};
  const Ogre::Image srcImg{getTexImage(srcPath)};
  const Ogre::PixelBox srcBox{srcImg.getPixelBox()};

  const uint32_t srcWidth{srcBox.getWidth()};
  const uint32_t srcHeight{srcBox.getHeight()};

  const auto toPixelsX = [srcWidth](float w) {
    return static_cast<uint32_t>(w * srcWidth);
  };

  const auto toPixelsY = [srcHeight](float h) {
    return static_cast<uint32_t>(h * srcHeight);
  };

  // Read the glyph data and record the largest width, the largest height, and
  // the ascent of the glyph with the largest height; this will be used to
  // position the baseline of each glyph.
  uint32_t maxGlyphWidth{0};
  uint32_t maxGlyphHeight{0};
  uint32_t ascentOfTallestGlyph{0};

  std::array<Glyph, 256> glyphs{};

  for (auto &g : glyphs) {
    fntPtr->read(reinterpret_cast<char *>(&g), sizeof(Glyph));

    if (auto w{toPixelsX(g.topRight.x - g.topLeft.x)}; w > maxGlyphWidth) {
      maxGlyphWidth = w;
    }

    if (auto h{toPixelsY(g.bottomLeft.y - g.topLeft.y)}; h > maxGlyphHeight) {
      maxGlyphHeight = h;
      ascentOfTallestGlyph = static_cast<uint32_t>(g.ascent);
    }
  }

  // Compute new texture size, assuming a square texture.
  // TODO: This seems to be a little large in practice.
  const uint32_t dstWidth = [&]() -> uint32_t {
    const uint32_t spacer{mFont->getCharacterSpacer()};
    const auto maxGlyphDim{std::max(maxGlyphHeight, maxGlyphWidth)};

    const auto area{(maxGlyphWidth + spacer) * (maxGlyphHeight + spacer) * 256};
    const auto widthBound{std::sqrt(area) + maxGlyphDim};
    const auto width{std::exp2(std::ceil(std::log2(widthBound)))};

    return static_cast<uint32_t>(width);
  }();
  const auto dstHeight{dstWidth};

  auto toNormalizedX = [dstWidth](uint32_t w) {
    return static_cast<float>(w) / static_cast<float>(dstWidth);
  };

  auto toNormalizedY = [dstHeight](uint32_t h) {
    return static_cast<float>(h) / static_cast<float>(dstHeight);
  };

  // Allocate data for the new texture
  const std::size_t numBytes{dstWidth * dstHeight * 4u};
  auto dstStreamPtr{std::make_shared<Ogre::MemoryDataStream>(numBytes)};
  auto dstPtr{dstStreamPtr->getPtr()};
  std::memset(dstPtr, 0, numBytes);
  Ogre::PixelBox dstBox(dstWidth, dstHeight, 1u, Ogre::PF_BYTE_RGBA, dstPtr);

  // Copy each glyph from the old image to the new image, inserting padding
  // using the ascender height.
  uint32_t left{0};
  uint32_t top{0};
  for (std::size_t i = 0; i < 256; ++i) {
    const auto &g{glyphs[i]};

    const Ogre::Box srcBounds{toPixelsX(g.topLeft.x),
                              toPixelsY(g.topLeft.y),
                              toPixelsX(g.bottomRight.x),
                              toPixelsY(g.bottomRight.y)};
    const Ogre::PixelBox src{srcBox.getSubVolume(srcBounds)};

    // Picture a box that is width * maxGlyphHeight. The baseline is placed
    // a distance ascentOfTallestGlyph from the top of the box. The current
    // glyph is placed within the box such that the distance from the top of the
    // glyph to the baseline is the glyph's ascent.
    const auto width{toPixelsX(g.topRight.x - g.topLeft.x)};
    const auto height{toPixelsY(g.bottomLeft.y - g.topLeft.y)};
    const auto ascent{static_cast<uint32_t>(g.ascent)};
    const auto glyphTop{top + ascentOfTallestGlyph - ascent};

    const Ogre::Box dstBounds{left, glyphTop, left + width, glyphTop + height};
    const Ogre::PixelBox dst{dstBox.getSubVolume(dstBounds)};

    // Copy the glyph over and update the font to point to it.
    Ogre::PixelUtil::bulkPixelConversion(src, dst);
    mFont->setGlyphTexCoords(WIN_1252[i],
                             toNormalizedX(left),
                             toNormalizedY(top),
                             toNormalizedX(left + width),
                             toNormalizedY(top + maxGlyphHeight),
                             static_cast<float>(dstWidth) / dstHeight);
    mFont->setGlyphAspectRatio(WIN_1252[i],
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
  imgPtrs.push_back(&dstImg);
  tex->_loadImages(imgPtrs);
}

} // namespace oo
