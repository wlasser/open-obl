#ifndef OPENOBL_GUI_SOUND_HPP
#define OPENOBL_GUI_SOUND_HPP

#include <string>

namespace gui {

/// Return the filepath of the ui sound with the given index.
/// Returns an empty string if the index does not correspond to a sound.
std::string getClicksound(int index);

} // namespace gui

#endif // OPENOBL_SOUND_HPP
