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

Currently only Linux with Clang 8+ is supported, though lower versions are
likely to work. Building with GCC 8.3+ should succeed, but there are some
outstanding gameplay bugs. Support for Windows is in-progress.

Unfortunately there are quite a few required dependencies with different methods
of installation. Several libraries are provided in the `lib` folder or
downloaded with CMake's FetchContent and built automatically. You should also
have installed (say through your system's package manager):

- CMake 3.13 or above
- Boost 1.67 or above
- SDL2 2.0 or above
- ZLIB 1.2.11 or above

Finally, there are some external dependencies that need to be built from source,
namely [my fork of OGRE](https://github.com/EmilyMansfield/ogre) and
[LLVM 7.1.0](https://llvm.org/).

### Building OGRE

Once you've cloned this repository into a directory `open-obl`, navigate to the
parent of that directory, download the fork of OGRE and switch branch:
```
git clone https://github.com/EmilyMansfield/ogre.git open-obl-ogre
cd open-obl-ogre
git checkout OpenOBL
```
Next, make a build directory and configure CMake. Many of the options below are
to explicitly disable functionality that isn't needed to speed up build times,
but doing so isn't necessary. The important things are that `OGRE_STATIC` is
set to `ON` and that a local install directory is specified, not the system
install directories.
```
mkdir Build
cd Build
CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Release \
                           -DCMAKE_INSTALL_PREFIX=../Install \
                           -DOGRE_STATIC=ON \
                           -DOGRE_ENABLE_PRECOMPILED_HEADER=OFF \
                           -DOGRE_BUILD_COMPONENT_BITES=OFF \
                           -DOGRE_BUILD_COMPONENT_CSHARP=OFF \
                           -DOGRE_BUILD_COMPONENT_HLMS=OFF \
                           -DOGRE_BUILD_COMPONENT_JAVA=OFF \
                           -DOGRE_BUILD_COMPONENT_RTSHADERSYSTEM=OFF \
                           -DOGRE_BUILD_RENDERSYSTEM_GL=OFF \
                           -DOGRE_CONFIG_ENABLE_ZIP=OFF \
                           -DOGRE_INSTALL_DOCS=OFF \
                           -DOGRE_INSTALL_SAMPLES=OFF \
                           -DOGRE_INSTALL_SAMPLES_SOURCE=OFF \
                           -DOGRE_INSTALL_TOOLS=OFF ..
```
Now build OGRE and install it to the local install directory specified.
```
make -j8
make install
```

### Building LLVM

The process here is basically as described in the most recent
[LLVM documentation](https://llvm.org/docs/GettingStarted.html), so long as you
build `7.1.0` and not trunk.

Navigate to the parent directory of `open-obl` as when install OGRE and clone
LLVM:
```
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
git checkout llvmorg-7.1.0
```
Next, make a build directory and configure CMake. By default LLVM will build as
static libraries, in which case you should set `OO_USE_SHARED_LLVM_LIBS` to
`OFF` when configuring OpenOBL later. This may not work, and for now shared
libraries should be used instead as is enabled below.
```
mkdir Build
cd Build
CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Release \
                           -DCMAKE_INSTALL_PREFIX=../Install \
                           -DLLVM_BUILD_LLVM_DYLIB=ON \
                           -DCMAKE_INSTALL_TESTS=OFF \
                           -DLLVM_BUILD_EXAMPLES=OFF \
                           -DLLVM_INCLUDE_EXAMPLES=OFF ..
```
If you run into issues building the tests, then they can be disabled by passing
`-DLLVM_BUILD_TESTS=OFF -DLLVM_INSTALL_TESTS=OFF` as additional options.
Now build and install LLVM to the local install directory:
```
make -j8
make install
```

### Building OpenOBL

With the dependencies installed, navigate into `open-obl` and make a build
directory, then configure CMake and build:
```
mkdir Build
cd Build
CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Release \
                           -DLLVM_ROOT=../../llvm-project/Install
                           -DOGRE_ROOT=../../open-obl-ogre/Install ..
make OpenOBL -j8
```

### Running Oblivion with OpenOBL

First, if you haven't already, go and buy Oblivion and install it somewhere, say
`$OBLIVION`. Run the game at least once so it can generate some configuration
files, and because it's fun to play.
Copy the built `OpenOBL` executable from `open-obl/Build/src/OpenOBL` into
`$OBLIVION`, then copy the `data` and `shaders` folders from `open-obl` into
$OBLIVION too.

Next you'll need to make a few changes to `Oblivion_default.ini`, or rename the
included `OpenOBL_default.ini` to `Oblivion_default.ini` and copy it into
`$OBLIVION`. If you choose to make changes,
- Find the `SMasterMismatchWarning` key and puts its value all on one line.
  The INI parser used by OpenOBL's parsing doesn't support multiline strings.
- Change the `SFontFile_x` keys to start with the `Fonts` directory, not with
  the `Data` directory.

Loading save games is only partially supported and saving is not yet supported.
If you want to load an existing save game from Oblivion, then please see the
documentation for `oo::GameSettings` (in `include/config/game_settings.hpp`) for
information on where save games should be placed. Otherwise, you can set the
INI setting `SStartingCell` to `0x00031bf9` to force load into the game world.
