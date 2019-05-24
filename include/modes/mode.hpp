#ifndef OPENOBL_MODE_HPP
#define OPENOBL_MODE_HPP

#include <optional>
#include <tuple>
#include <variant>

/// \file mode.hpp
/// \defgroup OpenOBLModes Game Modes
/// State machine for the game state handling menu changes etc.
///
/// The game state is split into three distinct state groups; oo::GameMode,
/// where the player is exploring the game world; oo::MenuMode, where the player
/// is navigating a menu such as the title screen, their inventory, or the game
/// options; and oo::ConsoleMode, when the developer console is open.
///
/// The oo::MenuMode state is split further into many closely-related states,
/// with each substate representing a particular menu. Each menu shares a common
/// backend engine provided by \ref OpenOBLGui, but differs in the meaning
/// of each interactable uiElement.
///
/// We will refer to oo::GameMode, oo::ConsoleMode, and all the substates of
/// oo::MenuMode as `Mode`s. The oo::Application stores a stack of `Mode`s and
/// dispatches much of its work for each frame to the `Mode::handleEvent()` and
/// `Mode::update()` member functions of the `Mode` on the top of the stack.
///
/// Since most `Mode` changes (i.e. state transitions) occur in response to user
/// input, the main method of changing game state is to return an
/// `oo::ModeTransition` from the `Mode::handleEvent()` method. Since a `Mode`
/// can transition to different `Mode`s in different circumstances (or more
/// frequently, not transition at all) the return type of `Mode::handleEvent()`
/// is not a single `Mode` but rather contains a variant of all the `Mode`s that
/// the currently executing `Mode` could ever transition to. At runtime, one of
/// those `Mode`s is selected and returned in the variant, or no `Mode` is
/// selected at all.
///
/// A `Mode` should model the following concept:
/// ```cpp
/// template<class T>
/// concept Mode = std::Constructible<T, ApplicationContext &> &&
///     requires(T x, ApplicationContext &ctx,
///              const sdl::Event &event, float delta) {
///     typename T::transition_t;
///
///     // Called when the Mode is pushed onto the mode stack.
///     { x.enter(ctx) } -> void;
///
///     // Called when the Mode becomes the top element of the mode stack.
///     { x.refocus(ctx) } -> void;
///
///     // Process the given SDL event, possibly returning a new `Mode`.
///     // Called at the start of oo::Application::frameStarted for every
///     // outstanding SDL event.
///     { x.handleEvent(ctx, event) } -> typename T::transition_t;
///
///     // Step the game state forward `delta` seconds.
///     // Called during oo::Application::frameStarted after all events have
///     // been processed.
///     { x.update(ctx, delta) -> void;
/// ```
/// \todo C++20: Move the concept out of documentation and into code.
namespace oo {

/// Data structure representing a transition from one `Mode` to another from the
/// set of `States`.
/// The first argument of the tuple specifies whether the currently executing
/// `Mode` should be popped from the mode stack during the transition.
/// The second argument represents an optional `Mode` to push on top of the
/// stack during the transition.
/// \ingroup OpenOBLModes
template<class ...States>
using ModeTransition = std::tuple<bool, std::optional<std::variant<States...>>>;

/// Type trait representing whether the `Mode` at the top of the stack should
/// hide its overlay (if any) when the given `Mode` is being pushed.
/// The motivation for this comes from the
/// `MainMenuMode -> LoadMenuMode -> LoadingMenuMode -> GameMode` transitions.
/// In the first transition `MainMenuMode` should not hide its overlay, because
/// `LoadMenuMode` does not fill the screen. In the second transition
/// `LoadMenuMode` pops itself from the stack and pushes `LoadingMenuMode`,
/// which fills the screen and therefore doesn't care that `MainMenuMode` is
/// still showing. In the third transition `LoadingMenuMode` pops itself and
/// pushes `GameMode`, but now `MainMenuMode`'s overlay is still visible. This
/// can be avoided if in the second transition `LoadingMenuMode` can tell
/// `MainMenuMode` to hide its overlay. In some sense the transition to
/// `LoadingMenuMode` then comes from `MainMenuMode`, even though `refocus` is
/// never called.
template<class Mode>
struct HideOverlayOnTransition : std::false_type {};

} // namespace oo

#endif // OPENOBL_MODE_HPP
