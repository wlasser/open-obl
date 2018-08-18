#include "bsa.hpp"
#include "engine/bsa.hpp"
#include <ctime>
#include <filesystem>
#include <functional>
#include <optional>

namespace engine {

namespace {

class BSAArchive : public Ogre::Archive {
 private:
  // BSAReader loads on construction, but we want to defer reading the archive
  // until the load function is called, then support unloading the resource by
  // deleting the reader.
  std::string name = "";
  std::optional<bsa::BSAReader> reader = std::nullopt;

  template<class T>
  std::shared_ptr<std::vector<T>>
  find(const Ogre::String &pattern,
       bool recursive,
       bool dirs,
       const std::function<T(std::filesystem::path)> &f) const;

  Ogre::FileInfo getFileInfo(const std::filesystem::path &path) const;

 public:
  BSAArchive(const Ogre::String &name, const Ogre::String &archType);
  ~BSAArchive() override;

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

class BSADataStream : public Ogre::DataStream {
 private:
  // FileData is not closeable since it uses RAII. Why would you want an object
  // in an unusable but defined state? We use an optional to simulate this.
  // TODO: Check this state in most operations and throw a custom error
  // Ogre requires tell to be const, because obviously checking the stream
  // position doesn't modify the data, but for std::istream, the corresponding
  // tellg *does* modify the stream by setting failbit/badbit. Hence 'mutable'.
  // TODO: Replace bsa::FileData with its unique_ptr and a position indicator
  mutable std::optional<bsa::FileData> fileData;
 public:
  BSADataStream(const Ogre::String &name, bsa::FileData &&fileData);
  ~BSADataStream() override;
  void close() override;
  bool eof() const override;
  std::size_t read(void *buf, std::size_t count) override;
  void seek(std::size_t pos) override;
  void skip(long count) override;
  std::size_t tell() const override;
};
}

template<class T>
std::shared_ptr<std::vector<T>>
BSAArchive::find(const Ogre::String &pattern,
                 bool recursive,
                 bool dirs,
                 const std::function<T(std::filesystem::path)> &f) const {
  if (!reader) throw std::runtime_error("Archive is not loaded");

  // If the pattern involves a folder, then we match both the folder and the
  // filename, otherwise we match only the filename in any folder.
  bool fileOnly = std::filesystem::path(pattern).remove_filename().empty();

  std::shared_ptr<std::vector<T>> ret(new std::vector<T>);

  for (const auto &folder : *reader) {
    if (dirs) {
      // Only want to check directories, not files
      if (Ogre::StringUtil::match(folder.name, pattern, isCaseSensitive())) {
        ret->push_back(f(folder.name));
      }
    } else {
      std::filesystem::path folderPath{folder.name};
      // Want to check for files
      for (const auto &file : folder.files) {
        auto path = fileOnly ? file : (folderPath / file).string();
        if (Ogre::StringUtil::match(path, pattern, isCaseSensitive())) {
          ret->push_back(f(folderPath / file));
        }
      }
    }
  }

  return ret;
}

Ogre::FileInfo BSAArchive::getFileInfo(const std::filesystem::path &path) const {
  if (!reader) throw std::runtime_error("Archive is not loaded");

  Ogre::FileInfo info;
  info.archive = this;
  info.filename = path;
  // It's not clear from the documentation what 'basename', 'filename', and
  // 'path' mean, so we let StringUtils deal with it.
  Ogre::StringUtil::splitFilename(path, info.basename, info.path);
  if (path.has_filename()) {
    // BSAReader transparently decompresses data, so it will appear to the user
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

BSAArchive::~BSAArchive() {
  unload();
}

[[noreturn]] Ogre::DataStreamPtr BSAArchive::create(const Ogre::String &filename) {
  throw std::runtime_error("Cannot modify BSA archives");
}

[[noreturn]] void BSAArchive::remove(const Ogre::String &filename) {
  throw std::runtime_error("Cannot modify BSA archives");
}

bool BSAArchive::exists(const Ogre::String &filename) const {
  if (!reader) throw std::runtime_error("Archive is not loaded");
  std::filesystem::path path(filename);
  auto file = path.filename();
  auto folder = path.remove_filename();
  return reader->contains(folder, file);
}

Ogre::StringVectorPtr BSAArchive::find(const Ogre::String &pattern,
                                       bool recursive, bool dirs) const {
  return find<std::string>(pattern, recursive, dirs,
                           [](std::filesystem::path path) {
                             return path.string();
                           });
}

Ogre::FileInfoListPtr BSAArchive::findFileInfo(const Ogre::String &pattern,
                                               bool recursive,
                                               bool dirs) const {
  return find<Ogre::FileInfo>(pattern, recursive, dirs,
                              [this](std::filesystem::path path) {
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
  auto path = std::filesystem::path(filename);
  auto file = path.filename();
  auto folder = path.remove_filename();
  return std::make_shared<BSADataStream>(filename, (*reader)[folder][file]);
}

std::time_t BSAArchive::getModifiedTime(const Ogre::String &filename) const {
  // BSA files don't track modification time, best we could do would be the
  // modification time of the entire archive, but bsa::BSAReader doesn't track
  // that so we'll just return the epoch.
  return 0;
}

bool BSAArchive::isCaseSensitive() const {
  return false;
}

bool BSAArchive::isReadOnly() const {
  return true;
}

BSADataStream::BSADataStream(const Ogre::String &name,
                             bsa::FileData &&fileData)
    : Ogre::DataStream(name), fileData(std::move(fileData)) {}

BSADataStream::~BSADataStream() = default;

void BSADataStream::close() {
  fileData.reset();
}

bool BSADataStream::eof() const {
  return !fileData || fileData->eof();
}

std::size_t BSADataStream::read(void *buf, std::size_t count) {
  fileData->read(static_cast<char *>(buf), count);
  // cppreference: "Except in the constructors of std::streambuf, negative
  // values of std::streamsize are never used."
  return static_cast<std::size_t>(fileData->gcount());
}

void BSADataStream::seek(std::size_t pos) {
  fileData->seekg(pos);
}

void BSADataStream::skip(long count) {
  fileData->seekg(fileData->tellg() + count);
}

std::size_t BSADataStream::tell() const {
  return static_cast<std::size_t>(fileData->tellg());
}

Ogre::Archive *BSAArchiveFactory::createInstance(const Ogre::String &name,
                                                 bool readOnly) {
  if (!readOnly) return nullptr;
  return new BSAArchive(name, getType());
}

void BSAArchiveFactory::destroyInstance(Ogre::Archive *ptr) {
  delete ptr;
}

const Ogre::String &BSAArchiveFactory::getType() const {
  static Ogre::String type = "BSA";
  return type;
}

} // namespace engine
