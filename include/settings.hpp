#ifndef OPENOBLIVION_SETTINGS_HPP
#define OPENOBLIVION_SETTINGS_HPP

/// \file
/// I don't like that this file exists; it's only purpose is to hold some
/// essentially unrelated constants which are needed throughout the codebase,
/// but don't really belong anywhere specific.

namespace oo {

const char *const RESOURCE_GROUP{"OOResource"};
const char *const SHADER_GROUP{"OOShader"};
const char *const RENDER_TARGET{"Open Oblivion"};
const char *const LOG{"OO"};
const char *const OGRE_LOG{"Ogre"};
const char *const DEFAULT_INI{"Oblivion_default.ini"};
const char *const USER_INI{"Oblivion.ini"};
const char *const APPLICATION_NAME{"OpenOblivion"};

} // namespace oo

#endif // OPENOBLIVION_SETTINGS_HPP
