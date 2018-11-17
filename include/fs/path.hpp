#ifndef OPENOBLIVION_FS_PATH_HPP
#define OPENOBLIVION_FS_PATH_HPP

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace fs {

/// Lightweight alternative to dealing with std::filesystem::path when paths are
/// very simple. This class represents a case-insensitive file or directory path
/// inside some unknown directory, with `'/'` and `'\\'` as the directory
/// separators. `'.'` and `'..'` do not have their usual meanings; they are
/// stripped from the beginning and end of the path along with any `'/'` and
/// treated as any other character everywhere else in the path. For example,
/// `'./foo\\.bar/..\\.'` and `'foo/.bar'` are the same. Every path is
/// also assumed to contain only ascii characters.
class Path {
 private:
  using size_type = std::string::size_type;
  /// Lowercase normalized path with `'/'` separators and no trailing `'/'`
  /// or `'.'`.
  /*const*/ std::string mPath{};
  mutable std::optional<std::filesystem::path> mSysPath{};
  mutable std::mutex mSysPathMutex{};

 public:
  Path(const Path &other);
  Path &operator=(const Path &other);

  Path(Path &&other) noexcept;
  Path &operator=(Path &&other) noexcept;

  Path() = default;

  /// Store a normalized representation of the given path
  explicit Path(const std::string &path);

  /// Return the part of the path after the last `'/'`, not including the `'/'`.
  /// This may not return an actual file, for example if the path points to a
  /// folder. If there is no `'/'` then the entire path is returned.
  /// The returned path is always lowercase.
  std::string_view filename() const;

  /// Return the complement of filename() without the trailing `'/'`.
  /// The returned path is always lowercase.
  std::string_view folder() const;

  /// Return the part of the filename strictly after the last '.' that does not
  /// occur at the beginning of the filename, i.e. including the '.' itself.
  /// Note that this is different to std::filesystem::path::extension, which
  /// includes the '.'.
  std::string_view extension() const;

  /// Return true if this path refers to an existing file or folder. Because of
  /// case-insensitivity, the file/folder need not be unique.
  /// \warning This is not a cheap operation on first run.
  bool exists() const;

  /// Return the path of an actual file or folder on the system whose Path is
  /// the same as this. If there are multiple such files/folders, the returned
  /// file/folder is unspecified, but will always be the same for repeated calls
  /// on the same Path.
  /// \exception std::runtime_error Thro if no such file/folder exists.
  std::filesystem::path sysPath() const;

  /// Return true if this path matches the given pattern.
  /// A `'*'` in the pattern means to greedily match until a character equal to
  /// the subsequent character in the pattern is found. If `'*'` occurs at the
  /// end of the pattern, then the rest of the path is matched automatically.
  /// The subsequene `'**'` of a pattern acts consistenly; it will match every
  /// character in the path until a literal '*' is found. There is no way to
  /// explictly match for a literal `'*'`.
  bool match(const fs::Path &pattern) const;

  inline std::string_view view() const {
    return std::string_view{mPath};
  };

  inline const char *c_str() const {
    return mPath.c_str();
  }

  inline bool empty() const {
    return mPath.empty();
  }

  inline bool has_filename() const {
    return filename().empty();
  }

  inline bool hash_extension() const {
    return extension().empty();
  }

  friend Path operator/(const Path &lhs, const Path &rhs);

  friend bool operator==(const Path &lhs, const Path &rhs);

  friend bool operator!=(const Path &lhs, const Path &rhs);
};

} // namespace fs

#endif // OPENOBLIVION_FS_PATH_HPP
