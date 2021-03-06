#ifndef OPENOBL_SCRIPT_ENGINE_HPP
#define OPENOBL_SCRIPT_ENGINE_HPP

#include "scripting/script_engine_base.hpp"
#include <nostdx/propagate_const.hpp>
#include <optional>

namespace oo {

/// Compilers user scripts and makes them available for running at game time.
class ScriptEngine : public ScriptEngineBase {
 private:
  class Impl;
  nostdx::propagate_const<std::unique_ptr<Impl>> mImpl;

  [[nodiscard]] std::optional<llvm::JITTargetAddress>
  getFunctionAddr(const std::string &scriptName, const std::string &funName);

 public:
  /// Compile a script into native object code, making it available for calling.
  /// If `calleeRef` is given then free function calls that do not resolve to
  /// known functions implicitly take `calleeRef` as their first argument, if
  /// the function would then resolve.
  void compile(std::string_view script, std::optional<uint32_t> calleeRef = {});

  ScriptEngine();
  ~ScriptEngine();
  ScriptEngine(const ScriptEngine &) = delete;
  ScriptEngine &operator=(const ScriptEngine &) = delete;
  ScriptEngine(ScriptEngine &&) noexcept;
  ScriptEngine &operator=(ScriptEngine &&) noexcept;

  /// Call the given function from the given script.
  /// The given script must been `compile`d previously, and a function with the
  /// given name and specified return type must exist in the script.
  /// \tparam T The return type of the function.
  // TODO: Make this take std::string_view with a conversion to llvm::StringRef
  template<class T> auto
  call(const std::string &scriptName, const std::string &funName)
  -> std::conditional_t<std::is_same_v<T, void>, void, std::optional<T>>;

  template<class Fun> void registerFunction(const std::string &funName);
};

template<class T> auto
ScriptEngine::call(const std::string &scriptName, const std::string &funName)
-> std::conditional_t<std::is_same_v<T, void>, void, std::optional<T>> {
  auto addr{getFunctionAddr(scriptName, funName)};

  if constexpr (std::is_same_v<T, void>) {
    if (!addr) return;
    std::uintptr_t addrPtr{*addr};
    auto fun{reinterpret_cast<void (*)()>(addrPtr)};
    fun();
    return;
  } else {
    if (!addr) return std::nullopt;
    std::uintptr_t addrPtr{*addr};
    auto fun{reinterpret_cast<T (*)()>(addrPtr)};
    return fun();
  }
}

template<class Fun>
void ScriptEngine::registerFunction(const std::string &funName) {
  addExternalFun<Fun>(funName);
}

} // namespace oo

#endif //OPENOBL_SCRIPT_ENGINE_HPP
