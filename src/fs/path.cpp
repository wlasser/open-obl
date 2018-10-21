#include "fs/path.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <stdexcept>

namespace fs {

Path::Path(const Path &other) {
  std::unique_lock lock{other.mSysPathMutex};
  mPath = other.mPath;
  mSysPath = other.mSysPath;
}

Path &Path::operator=(const Path &other) {
  if (*this != other) {
    std::unique_lock writeLock{mSysPathMutex, std::defer_lock};
    std::unique_lock readLock{other.mSysPathMutex, std::defer_lock};
    std::lock(writeLock, readLock);
    mPath = other.mPath;
    mSysPath = other.mSysPath;
  }
  return *this;
}

Path::Path(Path &&other) noexcept {
  std::unique_lock lock{other.mSysPathMutex};
  std::swap(mPath, other.mPath);
  std::swap(mSysPath, other.mSysPath);
}

Path &Path::operator=(Path &&other) noexcept {
  if (*this != other) {
    std::unique_lock writeLock{mSysPathMutex, std::defer_lock};
    std::unique_lock readLock{other.mSysPathMutex, std::defer_lock};
    std::lock(writeLock, readLock);
    std::swap(mPath, other.mPath);
    std::swap(mSysPath, other.mSysPath);
  }
  return *this;
}

// Store a normalized representation of the given path
Path::Path(const std::string &path) {
  mPath.reserve(path.size());
  std::transform(path.begin(), path.end(), std::back_inserter(mPath),
      // We only care about ascii, this is much faster than tolower.
                 [](unsigned char c) -> unsigned char {
                   if ('A' <= c && c <= 'Z') return c - 'A' + 'a';
                   else if (c == '\\') return '/';
                   else return c;
                 });
  // Drop all trailing / and .
  boost::algorithm::trim_if(mPath, boost::algorithm::is_any_of("/."));
}

// Return the part of the path after the last '/', not including the '/'.
// This may not be an actual file, for example if the path points to a folder.
// If there is no '/' then the entire path is returned.
std::string_view Path::filename() const {
  const std::string_view sv{mPath};
  // mPath does not end with a '/', but it might not have any at all
  const auto sep{sv.find_last_of('/')};
  return sep == std::string_view::npos ? sv : sv.substr(sep + 1);
};

// Return the complement of filename() without the trailing '/'
std::string_view Path::folder() const {
  const std::string_view sv{mPath};
  const auto sep{sv.find_last_of('/')};
  return sep == std::string_view::npos ? "" : sv.substr(0, sep);
};

// Return the part of the filename strictly after the last '.' that does not
// occur at the beginning of the filename, i.e. including the '.' itself.
// Note that this is different to std::filesystem::path::extension, which
// includes the '.'.
std::string_view Path::extension() const {
  auto sv{filename()};
  if (sv.empty()) return sv;
  const auto dot{sv.find_last_of('.')};
  // If the '.' is at the beginning, or there was no '.', then there is no
  // extension.
  if (dot == std::string_view::npos || dot == 0) return "";
  else return sv.substr(dot + 1);
};

// Return true if this path refers to an existing file or folder. Because of
// case-insensitivity, the file/folder need not be unique.
// This is not a cheap operation.
bool Path::exists() const {
  namespace stdfs = std::filesystem;
  // Iteration order is unspecified, so we cannot assume that it is the same
  // each time. This means that two threads calling exists() simultaneously
  // may return different files, and hence we have to lock immediately.
  std::unique_lock lock{mSysPathMutex};
  if (mSysPath) return true;

  for (const auto &entry : stdfs::recursive_directory_iterator(".")) {
    const Path entryPath{entry.path()};
    if (entryPath == *this) {
      mSysPath = entry.path();
      return true;
    }
  }
  return false;
}

// Return the path of an actual file or folder on the system whose Path is the
// same as this. If there are multiple such files/folders, the returned
// file/folder is unspecified, but will always be the same for repeated calls
// on the same Path. If no such file/folder exists, throw.
std::filesystem::path Path::sysPath() const {
  if (mSysPath) {
    return *mSysPath;
  } else if (exists()) {
    return *mSysPath;
  } else {
    throw std::runtime_error("No such file or folder");
  }
}

Path operator/(const Path &lhs, const Path &rhs) {
  return Path{lhs.mPath + '/' + rhs.mPath};
};

bool operator==(const Path &lhs, const Path &rhs) {
  return lhs.mPath == rhs.mPath;
}

bool operator!=(const Path &lhs, const Path &rhs) {
  return !(lhs == rhs);
}

} // namespace fs