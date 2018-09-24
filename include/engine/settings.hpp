#ifndef OPENOBLIVION_ENGINE_SETTINGS_HPP
#define OPENOBLIVION_ENGINE_SETTINGS_HPP

#include <string>

// I don't like that this file exists; it's only purpose is to hold some
// essentially unrelated constants which are needed throughout the codebase,
// but don't really belong anywhere specific.
// Should these be constexpr std::string_view?
namespace engine::settings {

const char *const resourceGroup{"OOResource"};
const char *const log{"OO"};
const char *const ogreLog{"Ogre"};

} // namespace engine::settings

#endif // OPENOBLIVION_ENGINE_SETTINGS_HPP
