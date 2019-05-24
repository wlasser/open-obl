#ifndef OPENOBL_OGRE_TEX_IMAGE_CODEC_HPP
#define OPENOBL_OGRE_TEX_IMAGE_CODEC_HPP

#include <OgreImageCodec.h>

namespace Ogre {

/// Codec for the very simple `.tex` image format used for storing bitmap fonts.
/// A `.tex` file consists of two little-endian `uint32_t`s representing the
/// image width and height in pixels, followed by a sequence of 4-byte pixels
/// with 8-bit depth in RGBA order.
/// \warning Little-endian architecture is assumed, big-endian is not supported.
class TexImageCodec : public ImageCodec {
 private:
  void encodeToFileStream(const String &filename,
                          const unsigned char *data,
                          uint32_t width,
                          uint32_t height) const;

 public:
  ~TexImageCodec() override {}

  ImageCodec::DecodeResult decode(const DataStreamPtr &input) const override;

  DataStreamPtr encode(const MemoryDataStreamPtr &input,
                       const CodecDataPtr &pData) const override;

  void encodeToFile(const MemoryDataStreamPtr &input,
                    const String &outFilename,
                    const CodecDataPtr &pData) const override;

  String getType() const override {
    return "tex";
  }

  /// This format does not support magic numbers, so this always returns false.
  bool magicNumberMatch(const char */*magicNumberPtr*/,
                        std::size_t /*maxBytes*/) const override {
    return false;
  }

  /// This format does not support magic numbers, so this always returns an
  /// empty string.
  String magicNumberToFileExt(const char */*magicNumberPtr*/,
                              std::size_t /*maxBytes*/) const override {
    return "";
  }
};

} // namespace Ogre

#endif // OPENOBL_OGRE_TEX_IMAGE_CODEC_HPP
