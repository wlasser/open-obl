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

Path::Path(const std::string &path) {
  // This might overshoot a little due to the trim in the next step
  mPath.reserve(path.size());
  // Drop all trailing / and .
  boost::algorithm::trim_copy_if(std::back_inserter(mPath), path,
                                 boost::algorithm::is_any_of("\\/."));
  // Lowercase
  std::transform(mPath.begin(), mPath.end(), mPath.begin(),
      // We only care about ascii, this is much faster than tolower.
                 [](unsigned char c) -> unsigned char {
                   if ('A' <= c && c <= 'Z') return c - 'A' + 'a';
                   else if (c == '\\') return '/';
                   else return c;
                 });
}

std::string_view Path::filename() const {
  const std::string_view sv{mPath};
  // mPath does not end with a '/', but it might not have any at all
  const auto sep{sv.find_last_of('/')};
  return sep == std::string_view::npos ? sv : sv.substr(sep + 1);
};

std::string_view Path::folder() const {
  const std::string_view sv{mPath};
  const auto sep{sv.find_last_of('/')};
  return sep == std::string_view::npos ? "" : sv.substr(0, sep);
};

std::string_view Path::extension() const {
  auto sv{filename()};
  if (sv.empty()) return sv;
  const auto dot{sv.find_last_of('.')};
  // If the '.' is at the beginning, or there was no '.', then there is no
  // extension.
  if (dot == std::string_view::npos || dot == 0) return "";
  else return sv.substr(dot + 1);
};

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

std::filesystem::path Path::sysPath() const {
  if (mSysPath) {
    return *mSysPath;
  } else if (exists()) {
    return *mSysPath;
  } else {
    throw std::runtime_error("No such file or folder");
  }
}

bool Path::match(const fs::Path &pattern) const {
  // Every character in the pattern corresponds to at least one in the src.
  if (pattern.mPath.size() > mPath.size()) return false;

  auto srcIt{mPath.begin()};
  const auto srcEnd{mPath.end()};

  auto patIt{pattern.mPath.begin()};
  const auto patEnd{pattern.mPath.end()};

  for (; srcIt != srcEnd && patIt != patEnd; ++srcIt) {
    if (*patIt == '*') {
      if (const auto nextIt{patIt + 1}; nextIt == patEnd) {
        // Last character is a wildcard so this is a match
        return true;
      } else if (*nextIt == *srcIt) {
        // src matches the character after the wildcard so stop wildcard match
        patIt += 2;
        continue;
      }
      // Don't advance the pattern, just the src
      continue;
    } else if (*patIt != *srcIt) {
      // src does not patch pattern and pattern is not a wildcard, so fail
      return false;
    }
    // Non-wildcard so advance both pattern and src
    ++patIt;
  }
  // Pattern does not end in a star and we cannot run out of src before running
  // out of pattern. If both run out at once, it's a match, otherwise the src is
  // longer than the pattern and there's no match.
  return srcIt == srcEnd && patIt == patEnd;
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