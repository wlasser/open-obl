#ifndef OPENOBLIVION_CONSOLE_FUNCTIONS_HPP
#define OPENOBLIVION_CONSOLE_FUNCTIONS_HPP

/// \defgroup OpenOblivionConsole
/// Scripting commands that can be run exclusively inside the developer console.
/// Because the scripting engine does not understand `void`, every function must
/// return a value. The convention used is that if the function does not have an
/// explicit meaning attached to its return value, then it shall return an error
/// code of type `int`. A return value of zero indicates success, and any other
/// return value (if the function even returns at all) indicates failure.
///@{
namespace console {

/// Quit the game, performing the usual cleanup.
extern "C" int QuitGame();

/// Shorthand for QuitGame()
/// \see console::QuitGame()
extern "C" int qqq();

} // namespace console
///@}

#endif // OPENOBLIVION_CONSOLE_FUNCTIONS_HPP
