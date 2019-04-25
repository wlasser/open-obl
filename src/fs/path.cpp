#include "fs/path.hpp"
#include <stdexcept>

namespace oo {

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
  if (path.empty()) {
    mPath = path;
  } else {
    // This might overshoot a little due to the trim in the next step
    mPath.reserve(path.size());
    // Drop all trailing / and .
    trim_copy(path.begin(), path.end(), std::back_inserter(mPath), [](char c) {
      return c == '\\' || c == '/' || c == '.';
    });
    // Lowercase
    std::transform(mPath.begin(), mPath.end(), mPath.begin(),
        // We only care about ascii, this is much faster than tolower.
                   [](unsigned char c) -> unsigned char {
                     if ('A' <= c && c <= 'Z') return c - 'A' + 'a';
                     else if (c == '\\') return '/';
                     else return c;
                   });
  }
}

Path::Path(std::string &&path) : mPath(std::move(path)) {
  if (mPath.empty()) return;
  trim_inplace(mPath, [](char c) { return c == '\\' || c == '/' || c == '.'; });
  std::transform(mPath.begin(), mPath.end(), mPath.begin(),
                 [](unsigned char c) -> unsigned char {
                   if ('A' <= c && c <= 'Z') return c - 'A' + 'a';
                   else if (c == '\\') return '/';
                   else return c;
                 });
}

Path::Path(const std::string &path, prenormalized_path_tag_t) : mPath(path) {}

Path::Path(std::string &&path, prenormalized_path_tag_t)
    : mPath(std::move(path)) {}

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

bool Path::match(const oo::Path &pattern) const {
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

Path &Path::operator/=(const Path &rhs) {
  // By construction, both this and rhs are normalized and have no trailing
  // slashes. Unless rhs or this are empty, joining the two with a / keeps
  // normalization.
  if (empty()) {
    *this = rhs;
  } else if (!rhs.empty()) {
    mPath.reserve(mPath.size() + rhs.mPath.size() + 1u);
    mPath.append(1u, '/').append(rhs.mPath);
  }

  return *this;
}

Path operator/(const Path &lhs, const Path &rhs) {
  // The naive `Path tmp{lhs}; tmp /= rhs; return tmp;` allocates twice when
  // only one allocation is actually necessary.
  Path tmp{};
  tmp.mPath.reserve(lhs.mPath.size() + 1u + rhs.mPath.size());
  tmp.mPath.append(lhs.mPath).append(1u, '/').append(rhs.mPath);
  return tmp;
};

bool operator==(const Path &lhs, const Path &rhs) {
  return lhs.mPath == rhs.mPath;
}

bool operator!=(const Path &lhs, const Path &rhs) {
  return !(lhs == rhs);
}

oo::Path makeNormalPath(const oo::Path &diffusePath) {
  auto dotIndex{diffusePath.view().rfind('.')};
  if (dotIndex == std::string_view::npos) return diffusePath;

  std::string normalPath;
  normalPath.reserve(diffusePath.view().size() + 2u);
  normalPath.append(diffusePath.view());
  normalPath.insert(dotIndex, "_n");
  return oo::Path(std::move(normalPath), oo::prenormalized_path_tag);
}

std::string makeNormalPath(const std::string &diffusePath) {
  return oo::makeNormalPath(oo::Path{diffusePath}).c_str();
}

} // namespace oo