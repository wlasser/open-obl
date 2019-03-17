#ifndef OPENOBLIVION_CONSOLE_FUNCTIONS_HPP
#define OPENOBLIVION_CONSOLE_FUNCTIONS_HPP

/// \defgroup OpenOblivionConsole Developer Console
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

/// Toggle the display of a wireframe collision box for all the collision
/// objects in the scene.
extern "C" int ToggleCollisionGeometry();

/// Shorthand for ToggleCollisionGeometry()
/// \see console::ToggleCollisionGeometry()
extern "C" int tcg();

/// Open the MainMenu, closing the current console window.
extern "C" int ShowMainMenu();

/// Open the ClassMenu, closing the current console window.
extern "C" int ShowClassMenu();

/// Open the EnchantmentMenu, closing the current console window.
extern "C" int ShowEnchantmentMenu();

/// Open the MapMenu, closing the current console window.
extern "C" int ShowMap();

/// Open the RaceSexMenu, closing the current console window.
extern "C" int ShowRaceMenu();

/// Open the SpellMakingMenu, closing the current console window.
extern "C" int ShowSpellmaking();

} // namespace console
///@}

#endif // OPENOBLIVION_CONSOLE_FUNCTIONS_HPP
