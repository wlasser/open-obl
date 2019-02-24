#ifndef OPENOBLIVION_SCRIPT_FUNCTIONS_HPP
#define OPENOBLIVION_SCRIPT_FUNCTIONS_HPP

/// \ingroup OpenOblivionScripting
/// Scripting commands that can be run inside user-defined scripts.
/// The same requirement on return values applies as for the functions in
/// OpenOblivionConsole.
///@{
namespace script {

/// Return the time of day as a floating point number of hours since midnight.
/// \remark The decimal part of the number is not the number of minutes but
///         the fraction of the hour, so `3.5` is `03:30am`.
/// \returns the value of the `record::GLOB` `GameHour`.
/// \ingroup OpenOblivionConsole
extern "C" float GetCurrentTime();
}

#endif // OPENOBLIVION_SCRIPT_FUNCTIONS_HPP
