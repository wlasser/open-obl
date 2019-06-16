# OpenOBL

OpenOBL is a work-in-progress game engine designed to be capable of running,
amongst other things, the game Oblivion created by Bethesda Softworks. Assets
are not provided; you must own a copy of Oblivion yourself in order to play it
using this engine.

## Features

Here is a short list of what is and is not currently implemented. There is a lot
to do, so this list is necessarily incomplete.

#### Supported
- Loading of interior and exterior areas, including (almost all) static objects
  and dynamics objects affected by physics.
- An exterior day-night cycle.
- A scripting engine.
- Music.
- A UI engine, though many menus are inaccessible or incomplete.

#### Not supported
- Animation (in progress).
- NPCs and creatures.
- A weather system.
- Distant object rendering (distant terrain rendering is done).
- Foliage.
- Basically any gameplay whatsoever besides walking around and jumping.
  :disappointed:

## Building

Unfortunately, building is currently an involved process and most platforms are
not fully supported. As a summary,

- Linux with Clang 8+ is fully supported and the main development platform,
  though earlier versions of Clang are likely to work too.
- Linux with GCC 8.3+ builds successfully, but there is an outstanding unknown
  bug causing the physics engine to misbehave.
- Windows with MSVC 19.21+ builds successfully, but currently fails to start.
- Windows with MinGW is unsupported due to problems with `thread_local` storage
  that prevent `Boost::Fiber` from building on that platform.
- No other platforms have been tested.

Please see the file `INSTALL.md` for instructions on how to build OpenOBL and
its dependencies for your system.

