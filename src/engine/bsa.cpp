#include "bsa/bsa.hpp"
#include "engine/bsa.hpp"
#include "fs/path.hpp"
#include "ogre/ogre_stream_wrappers.hpp"
#include <gsl/gsl>
#include <ctime>
#include <functional>
#include <optional>

namespace engine {

namespace {

class BSAArchive : public Ogre::Archive {
 private:
  using BSAArchiveStream = Ogre::OgreStandardStream<bsa::FileData>;
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
  BSAArchive(const Ogre::String &name, const Ogre::String &archType);
  ~BSAArchive() override = default;

  BSAArchive(const BSAArchive &other) = delete;
  BSAArchive &operator=(const BSAArchive &other) = delete;
  BSAArchive(BSAArchive &&other) = delete;
  BSAArchive &operator=(BSAArchive &&other) = delete;

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

  Ogre::DataStreamPtr open(const Ogre::String &filename,
                           bool readOnly) const override;
};

}

template<class T>
std::shared_ptr<std::vector<T>>
BSAArchive::find(const Ogre::String &pattern,
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

Ogre::FileInfo BSAArchive::getFileInfo(const fs::Path &path) const {
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

BSAArchive::BSAArchive(const Ogre::String &name,
                       const Ogre::String &archType) :
    Ogre::Archive(name, archType), name(name) {}

[[noreturn]] Ogre::DataStreamPtr BSAArchive::create(const Ogre::String &filename) {
  throw std::runtime_error("Cannot modify BSA archives");
}

[[noreturn]] void BSAArchive::remove(const Ogre::String &filename) {
  throw std::runtime_error("Cannot modify BSA archives");
}

bool BSAArchive::exists(const Ogre::String &filename) const {
  if (!reader) throw std::runtime_error("Archive is not loaded");
  fs::Path path{filename};
  const auto file{path.filename()};
  const auto folder{path.folder()};
  return reader->contains(std::string{folder}, std::string{file});
}

Ogre::StringVectorPtr BSAArchive::find(const Ogre::String &pattern,
                                       bool recursive, bool dirs) const {
  return find<std::string>(pattern, recursive, dirs,
                           [](const fs::Path &path) -> std::string {
                             return path.c_str();
                           });
}

Ogre::FileInfoListPtr BSAArchive::findFileInfo(const Ogre::String &pattern,
                                               bool recursive,
                                               bool dirs) const {
  return find<Ogre::FileInfo>(pattern, recursive, dirs,
                              [this](const fs::Path &path) -> Ogre::FileInfo {
                                return getFileInfo(path);
                              });
}

Ogre::StringVectorPtr BSAArchive::list(bool recursive, bool dirs) const {
  return find("*", recursive, dirs);
}

Ogre::FileInfoListPtr BSAArchive::listFileInfo(bool recursive,
                                               bool dirs) const {
  return findFileInfo("*", recursive, dirs);
}

void BSAArchive::load() {
  reader.emplace(name);
}

void BSAArchive::unload() {
  reader.reset();
}

Ogre::DataStreamPtr BSAArchive::open(const Ogre::String &filename,
                                     bool /*readOnly*/) const {
  if (!exists(filename)) return std::shared_ptr<Ogre::DataStream>(nullptr);
  const fs::Path path{filename};
  const auto file{path.filename()};
  const auto folder{path.folder()};
  return std::make_shared<BSAArchiveStream>(filename,
                                            (*reader)[std::string{
                                                folder}][std::string{file}]);
}

std::time_t BSAArchive::getModifiedTime(const Ogre::String &filename) const {
  // BSA files don't track modification time, best we could do would be the
  // modification time of the entire archive, but bsa::BsaReader doesn't track
  // that so we'll just return the epoch.
  return 0;
}

bool BSAArchive::isCaseSensitive() const {
  return false;
}

bool BSAArchive::isReadOnly() const {
  return true;
}

gsl::owner<Ogre::Archive *>
BsaArchiveFactory::createInstance(const Ogre::String &name, bool readOnly) {
  if (!readOnly) return nullptr;
  return new BSAArchive(name, getType());
}

void BsaArchiveFactory::destroyInstance(gsl::owner<Ogre::Archive *> ptr) {
  delete ptr;
}

const Ogre::String &BsaArchiveFactory::getType() const {
  static const Ogre::String type{"BSA"};
  return type;
}

} // namespace engine
