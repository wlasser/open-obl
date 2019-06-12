#ifndef OPENOBL_SCRIPT_FUNCTIONS_HPP
#define OPENOBL_SCRIPT_FUNCTIONS_HPP

/// \ingroup OpenOBLScripting
/// Scripting commands that can be run inside user-defined scripts.
/// The same requirement on return values applies as for the functions in
/// OpenOBLConsole.
///
/// Functions added here should be also be added to `symbols.list` so that they
/// are correctly included in the symbol table of the built executable and
/// accessible by the LLVM JIT compiler.
///@{
namespace script {

/// Return the time of day as a floating point number of hours since midnight.
/// \remark The decimal part of the number is not the number of minutes but
///         the fraction of the hour, so `3.5` is `03:30am`.
/// \returns the value of the `record::GLOB` `GameHour`.
/// \ingroup OpenOBLConsole
extern "C" float GetCurrentTime();
}

#endif // OPENOBL_SCRIPT_FUNCTIONS_HPP
