#ifndef OPENOBLIVION_SETTINGS_HPP
#define OPENOBLIVION_SETTINGS_HPP

#include <string>

// I don't like that this file exists; it's only purpose is to hold some
// essentially unrelated constants which are needed throughout the codebase,
// but don't really belong anywhere specific.
// Should these be constexpr std::string_view?
namespace settings {

const char *const resourceGroup{"OOResource"};
const char *const log{"OO"};
const char *const ogreLog{"Ogre"};
const char *const defaultIni{"Oblivion_default.ini"};
const char *const userIni{"Oblivion.ini"};

} // namespace settings

#endif // OPENOBLIVION_SETTINGS_HPP
