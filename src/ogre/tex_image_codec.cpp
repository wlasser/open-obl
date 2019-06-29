#include "ogre/tex_image_codec.hpp"
#include <OgreDataStream.h>
#include <OgrePixelFormat.h>
#include <cstdint>
#include <fstream>

namespace Ogre {

ImageCodec::DecodeResult
TexImageCodec::decode(const DataStreamPtr &input) const {
  uint32_t width{0};
  if (input->read(reinterpret_cast<char *>(&width), 4) != 4) {
    OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                "Failed to decode image width",
                "TexImageCodec::decode");
  }

  uint32_t height{0};
  if (input->read(reinterpret_cast<char *>(&height), 4) != 4) {
    OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                "Failed to decode image height",
                "TexImageCodec::decode");
  }

  const std::size_t numPixels{width * height};
  const std::size_t numBytes{numPixels * 4u};

  auto imageData{std::make_shared<ImageCodec::ImageData>()};
  imageData->depth = 1;
  imageData->flags = PFF_HASALPHA | PFF_INTEGER;
  imageData->format = PF_BYTE_RGBA;
  imageData->height = height;
  imageData->width = width;
  imageData->size = numBytes;
  imageData->num_mipmaps = 0;

  auto output{std::make_shared<MemoryDataStream>(numBytes)};
  if (input->read(output->getPtr(), numBytes) != numBytes) {
    OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                "Failed to decode image, read insufficient bytes",
                "TexImageCodec::decode");
  }

  return DecodeResult{std::move(output), std::move(imageData)};
}

DataStreamPtr TexImageCodec::encode(const MemoryDataStreamPtr &input,
                                    const CodecDataPtr &pData) const {
  auto *imageData{dynamic_cast<ImageCodec::ImageData *>(pData.get())};
  if (!imageData) {
    OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                "CodecDataPtr does not point to ImageCodec::ImageData",
                "TexImageCodec::encode");
  }

  auto width{imageData->width};
  auto height{imageData->height};

  // If the input format agrees with the output format then we can shortcut
  // and just dump the input into the output.
  if (imageData->format == PF_BYTE_RGBA) {
    const std::size_t numPixels{width * height};
    const std::size_t numBytes{numPixels * 4u + 8u};
    auto output{std::make_shared<MemoryDataStream>(numBytes)};
    output->write(reinterpret_cast<const char *>(&width), 4u);
    output->write(reinterpret_cast<const char *>(&height), 4u);
    output->write(input->getPtr(), numPixels * 4u);
    return output;
  }

  // Otherwise use PixelBoxes to convert from the input format to the output
  // format.
  PixelBox inPixels(width, height, imageData->depth, imageData->format,
                    input->getPtr());
  PixelBox outPixels(width, height, 1, PF_BYTE_RGBA);

  const std::size_t numBytes{outPixels.getConsecutiveSize() + 8u};
  auto output{std::make_shared<MemoryDataStream>(numBytes)};
  output->write(reinterpret_cast<const char *>(&width), 4u);
  output->write(reinterpret_cast<const char *>(&height), 4u);

  outPixels.data = output->getPtr() + 8;
  PixelUtil::bulkPixelConversion(inPixels, outPixels);

  return output;
}

void TexImageCodec::encodeToFileStream(const String &filename,
                                       const unsigned char *data,
                                       uint32_t width,
                                       uint32_t height) const {
  std::ofstream output(filename, std::ios_base::binary);
  if (!output.is_open()) {
    OGRE_EXCEPT(Exception::ERR_CANNOT_WRITE_TO_FILE,
                "Failed to open image output file",
                "TexImageCodec::encodeToFile");
  }

  output.write(reinterpret_cast<const char *>(&width), 4u);
  output.write(reinterpret_cast<const char *>(&height), 4u);
  output.write(reinterpret_cast<const char *>(data), width * height * 4u);
}

void TexImageCodec::encodeToFile(const MemoryDataStreamPtr &input,
                                 const String &outFilename,
                                 const CodecDataPtr &pData) const {
  auto *imageData{dynamic_cast<ImageCodec::ImageData *>(pData.get())};
  if (!imageData) {
    OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                "CodecDataPtr does not point to ImageCodec::ImageData",
                "TexImageCodec::encodeToFile");
  }

  auto width{imageData->width};
  auto height{imageData->height};

  // If the input format agrees with the output format then we can shortcut
  // and just dump the input into the output.
  if (imageData->format == PF_BYTE_RGBA) {
    encodeToFileStream(outFilename, input->getPtr(), width, height);
    return;
  }

  // Otherwise use PixelBoxes to convert from the input format to the output
  // format.
  PixelBox inPixels(width, height, imageData->depth, imageData->format,
                    input->getPtr());
  PixelBox outPixels(width, height, 1, PF_BYTE_RGBA);

  const std::size_t numBytes{outPixels.getConsecutiveSize()};
  std::vector<unsigned char> outPixelsData(numBytes);
  outPixels.data = outPixelsData.data();
  PixelUtil::bulkPixelConversion(inPixels, outPixels);

  encodeToFileStream(outFilename, outPixelsData.data(), width, height);
}

} // namespace Ogre
