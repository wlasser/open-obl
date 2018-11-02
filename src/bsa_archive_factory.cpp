#include "bsa/bsa.hpp"
#include "bsa_archive_factory.hpp"
#include "fs/path.hpp"
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
  std::string name{};
  std::optional<bsa::BsaReader> reader{};

  template<class T>
  std::shared_ptr<std::vector<T>>
  find(const Ogre::String &pattern,
       bool recursive,
       bool dirs,
       const std::function<T(fs::Path)> &f) const;

  Ogre::FileInfo getFileInfo(const fs::Path &path) const;

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

}

template<class T>
std::shared_ptr<std::vector<T>>
BsaArchive::find(const Ogre::String &pattern,
                 bool recursive,
                 bool dirs,
                 const std::function<T(fs::Path)> &f) const {
  if (!reader) throw std::runtime_error("Archive is not loaded");

  // If the pattern involves a folder, then we match both the folder and the
  // filename, otherwise we match only the filename in any folder.
  fs::Path patternPath{pattern};
  const bool fileOnly{patternPath.folder().empty()};

  auto ret{std::make_shared<std::vector<T>>()};

  for (const auto &folder : *reader) {
    fs::Path folderPath{folder.name};
    if (dirs) {
      // Only want to check directories, not files
      if (Ogre::StringUtil::match(folder.name, pattern, isCaseSensitive())) {
        ret->push_back(f(folderPath));
      }
    } else {
      // Want to check for files
      for (const auto &file : folder.files) {
        const fs::Path filePath{file};
        const auto path{fileOnly ? filePath : (folderPath / filePath)};
        if (Ogre::StringUtil::match(path.c_str(), pattern, isCaseSensitive())) {
          ret->push_back(f(folderPath / filePath));
        }
      }
    }
  }

  return ret;
}

Ogre::FileInfo BsaArchive::getFileInfo(const fs::Path &path) const {
  if (!reader) throw std::runtime_error("Archive is not loaded");

  Ogre::FileInfo info;
  info.archive = this;
  info.filename = path.c_str();
  // It's not clear from the documentation what 'basename', 'filename', and
  // 'path' mean, so we let StringUtils deal with it.
  Ogre::StringUtil::splitFilename(path.c_str(), info.basename, info.path);
  if (path.has_filename()) {
    // BsaReader transparently decompresses data, so it will appear to the user
    // that all the data is uncompressed.
    info.uncompressedSize = (*reader)[info.path][info.basename].size();
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
    Ogre::Archive(name, archType), name(name) {}

[[noreturn]] Ogre::DataStreamPtr BsaArchive::create(const Ogre::String &filename) {
  throw std::runtime_error("Cannot modify BSA archives");
}

[[noreturn]] void BsaArchive::remove(const Ogre::String &filename) {
  throw std::runtime_error("Cannot modify BSA archives");
}

bool BsaArchive::exists(const Ogre::String &filename) const {
  if (!reader) throw std::runtime_error("Archive is not loaded");
  fs::Path path{filename};
  const auto file{path.filename()};
  const auto folder{path.folder()};
  return reader->contains(std::string{folder}, std::string{file});
}

Ogre::StringVectorPtr BsaArchive::find(const Ogre::String &pattern,
                                       bool recursive, bool dirs) const {
  return find<std::string>(pattern, recursive, dirs, [](const fs::Path &path) {
    return std::string{path.c_str()};
  });
}

Ogre::FileInfoListPtr BsaArchive::findFileInfo(const Ogre::String &pattern,
                                               bool recursive,
                                               bool dirs) const {
  return find<Ogre::FileInfo>(pattern, recursive, dirs,
                              [this](const fs::Path &path) {
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
  reader.emplace(name);
}

void BsaArchive::unload() {
  reader.reset();
}

Ogre::DataStreamPtr BsaArchive::open(const Ogre::String &filename,
                                     bool /*readOnly*/) const {
  if (!exists(filename)) return std::shared_ptr<Ogre::DataStream>(nullptr);
  const fs::Path path{filename};
  const auto file{path.filename()};
  const auto folder{path.folder()};
  return std::make_shared<BsaArchiveStream>(
      filename, (*reader)[std::string{folder}][std::string{file}]);
}

std::time_t BsaArchive::getModifiedTime(const Ogre::String &filename) const {
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
