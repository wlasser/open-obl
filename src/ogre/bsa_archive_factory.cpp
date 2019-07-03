#include "bsa/bsa.hpp"
#include "fs/path.hpp"
#include "ogre/bsa_archive_factory.hpp"
#include "ogre/ogre_stream_wrappers.hpp"
#include <gsl/gsl>
#include <ctime>
#include <functional>
#include <optional>

namespace Ogre {

namespace {

class BsaArchive : public Ogre::Archive {
 private:
  using BsaArchiveStream = Ogre::OgreStandardStream<bsa::FileData>;
  // BsaReader loads on construction, but we want to defer reading the archive
  // until the load function is called, then support unloading the resource by
  // deleting the reader.
  std::string mBsaName{};
  std::optional<bsa::BsaReader> mReader{};

  template<class T>
  std::shared_ptr<std::vector<T>>
  find(const Ogre::String &pattern,
       bool dirs,
       const std::function<T(oo::Path)> &f) const;

  Ogre::FileInfo getFileInfo(const oo::Path &path) const;

 public:
  BsaArchive(const Ogre::String &name, const Ogre::String &archType);
  ~BsaArchive() override = default;

  BsaArchive(const BsaArchive &other) = delete;
  BsaArchive &operator=(const BsaArchive &other) = delete;
  BsaArchive(BsaArchive &&other) = delete;
  BsaArchive &operator=(BsaArchive &&other) = delete;

  [[noreturn]] Ogre::DataStreamPtr create(const Ogre::String &filename) override;
  [[noreturn]] void remove(const Ogre::String &filename) override;

  bool exists(const Ogre::String &filename) const override;

  Ogre::StringVectorPtr find(const Ogre::String &pattern,
                             bool recursive,
                             bool dirs) const override;
  Ogre::FileInfoListPtr findFileInfo(const Ogre::String &pattern,
                                     bool recursive, bool dirs) const override;

  std::time_t getModifiedTime(const Ogre::String &filename) const override;

  bool isCaseSensitive() const override;
  bool isReadOnly() const override;

  Ogre::StringVectorPtr list(bool recursive, bool dirs) const override;
  Ogre::FileInfoListPtr listFileInfo(bool recursive, bool dirs) const override;

  void load() override;
  void unload() override;

  Ogre::DataStreamPtr
  open(const Ogre::String &filename, bool readOnly) const override;
};

template<class T>
std::shared_ptr<std::vector<T>>
BsaArchive::find(const Ogre::String &pattern,
                 bool dirs,
                 const std::function<T(oo::Path)> &f) const {
  if (!mReader) throw std::runtime_error("Archive is not loaded");

  // If the pattern involves a folder, then we match both the folder and the
  // filename, otherwise we match only the filename in any folder.
  oo::Path patternPath{pattern};
  const bool fileOnly{patternPath.folder().empty()};

  auto ret{std::make_shared<std::vector<T>>()};

  for (bsa::FolderView folder : *mReader) {
    // TODO: oo::Path(std::string_view) constructor
    oo::Path folderPath{std::string(folder.name())};
    if (dirs) {
      // Only want to check directories, not files
      if (folderPath.match(patternPath)) {
        ret->push_back(f(folderPath));
      }
    } else {
      // Want to check for files
      for (bsa::FileView file : folder) {
        // TODO: oo::Path(std::string_view) constructor
        const oo::Path filePath{std::string(file.name())};
        const auto path{fileOnly ? filePath : (folderPath / filePath)};
        if (path.match(patternPath)) {
          ret->push_back(f(folderPath / filePath));
        }
      }
    }
  }

  return ret;
}

Ogre::FileInfo BsaArchive::getFileInfo(const oo::Path &path) const {
  if (!mReader) throw std::runtime_error("Archive is not loaded");

  Ogre::FileInfo info;
  info.archive = this;
  info.filename = path.c_str();
  // It's not clear from the documentation what 'basename', 'filename', and
  // 'path' mean, so we let StringUtils deal with it.
  Ogre::StringUtil::splitFilename(path.c_str(), info.basename, info.path);
  if (path.has_filename()) {
    // BsaReader transparently decompresses data, so it will appear to the user
    // that all the data is uncompressed.
    info.uncompressedSize = mReader->uncompressedSize(info.path, info.basename);
    info.compressedSize = info.uncompressedSize;
  } else {
    // BSA archives do not have directory sizes. We could compute the total size
    // of all entries in the file, but that's not usually what this means.
    info.compressedSize = 0;
    info.uncompressedSize = 0;
  }
  return info;
}

BsaArchive::BsaArchive(const Ogre::String &name,
                       const Ogre::String &archType) :
    Ogre::Archive(name, archType), mBsaName(name) {}

[[noreturn]] Ogre::DataStreamPtr
BsaArchive::create(const Ogre::String &/*filename*/) {
  throw std::runtime_error("Cannot modify BSA archives");
}

[[noreturn]] void BsaArchive::remove(const Ogre::String &/*filename*/) {
  throw std::runtime_error("Cannot modify BSA archives");
}

bool BsaArchive::exists(const Ogre::String &filename) const {
  if (!mReader) throw std::runtime_error("Archive is not loaded");
  oo::Path path{filename};
  const auto file{path.filename()};
  const auto folder{path.folder()};
  return mReader->contains(std::string{folder}, std::string{file});
}

Ogre::StringVectorPtr BsaArchive::find(const Ogre::String &pattern,
                                       bool /*recursive*/, bool dirs) const {
  return find<std::string>(pattern, dirs, [](const oo::Path &path) {
    return std::string{path.c_str()};
  });
}

Ogre::FileInfoListPtr BsaArchive::findFileInfo(const Ogre::String &pattern,
                                               bool /*recursive*/,
                                               bool dirs) const {
  return find<Ogre::FileInfo>(pattern, dirs, [this](const oo::Path &path) {
    return getFileInfo(path);
  });
}

Ogre::StringVectorPtr BsaArchive::list(bool recursive, bool dirs) const {
  return find("*", recursive, dirs);
}

Ogre::FileInfoListPtr
BsaArchive::listFileInfo(bool recursive, bool dirs) const {
  return findFileInfo("*", recursive, dirs);
}

void BsaArchive::load() {
  mReader.emplace(mBsaName);
}

void BsaArchive::unload() {
  mReader.reset();
}

Ogre::DataStreamPtr BsaArchive::open(const Ogre::String &filename,
                                     bool /*readOnly*/) const {
  if (!exists(filename)) return std::shared_ptr<Ogre::DataStream>(nullptr);
  const oo::Path path{filename};
  const std::string file{path.filename()};
  const std::string folder{path.folder()};
  return std::make_shared<BsaArchiveStream>(filename,
                                            mReader->stream(folder, file));
}

std::time_t
BsaArchive::getModifiedTime(const Ogre::String &/*filename*/) const {
  // BSA files don't track modification time, best we could do would be the
  // modification time of the entire archive, but bsa::BsaReader doesn't track
  // that so we'll just return the epoch.
  return 0;
}

bool BsaArchive::isCaseSensitive() const {
  return false;
}

bool BsaArchive::isReadOnly() const {
  return true;
}

} // namespace

gsl::owner<Ogre::Archive *>
BsaArchiveFactory::createInstance(const Ogre::String &name, bool readOnly) {
  if (!readOnly) return nullptr;
  return new BsaArchive(name, getType());
}

void BsaArchiveFactory::destroyInstance(gsl::owner<Ogre::Archive *> ptr) {
  delete ptr;
}

const Ogre::String &BsaArchiveFactory::getType() const {
  static const Ogre::String type{"BSA"};
  return type;
}

} // namespace Ogre
