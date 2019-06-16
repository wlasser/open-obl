# Building OpenOBL

*The build process is currently quite involved, and streamlining it is a
priority.*

There are quite a few required dependencies with different methods of
installation. Several libraries are provided in the `lib` folder or downloaded
with CMake's `FetchContent` and built automatically, whereas others should be
installed manually. You should have installed (say through your system's
package manager):

- CMake 3.13 or above (3.14 or above if building with MSVC)
- Boost 1.67 or above
- Python 2.7 (required only for building LLVM)
- SDL2 2.0 or above
- ZLIB 1.2.11 or above

Finally, there are some external dependencies that need to be built from source,
namely [my fork of OGRE](https://github.com/EmilyMansfield/ogre) and
[LLVM 7.1.0](https://llvm.org/).

## Linux

In this section, commands to be entered in a shell will begin with `$ `.

### Building OGRE

Once you've cloned this repository into a directory `open-obl`, navigate to the
parent of that directory, download the fork of OGRE, and switch branch:
```Shell
$ git clone https://github.com/EmilyMansfield/ogre.git open-obl-ogre
$ cd open-obl-ogre
$ git checkout OpenOBL
```
Next, make a build directory and configure CMake. Many of the options below
disable functionality that isn't needed to speed up build times, but doing so
isn't necessary. The important things are that `OGRE_STATIC` is set to `ON` and
that a local install directory is specified, not the system install directories.
```Shell
$ mkdir Build
$ cd Build
$ CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Release              \
                           -DCMAKE_INSTALL_PREFIX=../Install         \
                           -DOGRE_STATIC=ON                          \
                           -DOGRE_BUILD_COMPONENT_BITES=OFF          \
                           -DOGRE_BUILD_COMPONENT_CSHARP=OFF         \
                           -DOGRE_BUILD_COMPONENT_HLMS=OFF           \
                           -DOGRE_BUILD_COMPONENT_JAVA=OFF           \
                           -DOGRE_BUILD_COMPONENT_PYTHON=OFF         \
                           -DOGRE_BUILD_COMPONENT_RTSHADERSYSTEM=OFF \
                           -DOGRE_BUILD_RENDERSYSTEM_GL=OFF          \
                           -DOGRE_BUILD_RENDERSYSTEM_GL3PLUS=ON      \
                           -DOGRE_BUILD_RENDERSYSTEM_GLES2=OFF       \
                           -DOGRE_BUILD_PLUGIN_BSP=OFF               \
                           -DOGRE_BUILD_PLUGIN_OCTREE=OFF            \
                           -DOGRE_BUILD_PLUGIN_PCZ=OFF               \
                           -DOGRE_CONFIG_ENABLE_ZIP=OFF              \
                           -DOGRE_INSTALL_DOCS=OFF                   \
                           -DOGRE_INSTALL_SAMPLES=OFF                \
                           -DOGRE_INSTALL_SAMPLES_SOURCE=OFF         \
                           -DOGRE_INSTALL_TOOLS=OFF                  \
                           ..
```
If you run into problems with precompiled headers then they can be turned off
using `-DOGRE_ENABLE_PRECOMPILED_HEADER=OFF`.

Now build OGRE and install it to the local install directory specified.
```Shell
$ make -j8
$ make install
```

### Building LLVM

The process here is essentially as described in the most recent
[LLVM documentation](https://llvm.org/docs/GettingStarted.html), so long as you
build `7.1.0` and not trunk.

Navigate to the parent directory of `open-obl` as when building OGRE and clone
LLVM:
```Shell
$ git clone https://github.com/llvm/llvm-project.git
$ cd llvm-project
$ git checkout llvmorg-7.1.0
```
Next, make a build directory and configure CMake. By default LLVM will build
static libraries, though you may choose to build shared libraries by setting
`-DLLVM_BUILD_LLVM_DYLIB=ON`, provided that you set `OO_USE_SHARED_LLVM_LIBS` to
`ON` when configuring OpenOBL later.
```Shell
$ mkdir Build
$ cd Build
$ CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Release        \
                             -DCMAKE_INSTALL_PREFIX=../Install \
                             -DCMAKE_INSTALL_TESTS=OFF         \
                             -DLLVM_BUILD_EXAMPLES=OFF         \
                             -DLLVM_INCLUDE_EXAMPLES=OFF       \
                             ..
```
If you run into issues building the tests, then they can be disabled by passing
`-DLLVM_BUILD_TESTS=OFF -DLLVM_INSTALL_TESTS=OFF` as additional options.

Now build and install LLVM to the local install directory:
```Shell
$ make -j8
$ make install
```

### Building OpenOBL

With the dependencies installed, navigate into `open-obl` and make a build
directory, then configure CMake and build:
```Shell
$ mkdir Build
$ cd Build
$ CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Release              \
                             -DLLVM_ROOT=../../llvm-project/Install  \
                             -DOGRE_ROOT=../../open-obl-ogre/Install \
                             ..
$ make OpenOBL -j8
```

## Windows

These instructions are for building on Windows from the command line using the
Visual Studio Build Tools. You will (probably, I'm new to Windows) need the
components
 - C++ 2019 Redistributable Update
 - C++ CMake tools (if you don't have CMake 3.14 installed already)
 - MSVC x64/x86 build tools
 - C++ Build Tools core features
 - C++ ATL (for the selected version of the MSVC build tools).
Commands beginning with `$ ` should be run in Git Bash, whereas commands
beginning with `> ` should be run in the [appropriate Build Tools Developer
Prompt](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=vs-2019#developer_command_prompt_shortcuts)
for your architecture. Note that on x64 the default prompt builds 32-bit code.

### Building OGRE

Once you've cloned this repository into a directory `open-obl`, navigate to the
parent of that directory, download the fork of OGRE, and switch branch:
```Shell
$ git clone https://github.com/EmilyMansfield/ogre.git open-obl-ogre
$ cd open-obl-ogre
$ git checkout OpenOBL
```
Next, navigate to `open-obl-ogre` in the developer prompt, make a build
directory, and configure CMake. Many of the options below disable functionality
that isn't needed to speed uo build times, but doing so isn't necessary. The
important things are that `OGRE_STATIC` is set to `ON`, that the generator is
set, and that a local install directory is specified.
```Shell
> mkdir Build
> cd Build
> cmake -G "Visual Studio 16 2019" ^
        -DCMAKE_BUILD_TYPE=Release ^
        -DCMAKE_INSTALL_PREFIX=../Install ^
        -DOGRE_STATIC=ON ^
        -DOGRE_BUILD_COMPONENT_BITES=OFF ^
        -DOGRE_BUILD_COMPONENT_CSHARP=OFF ^
        -DOGRE_BUILD_COMPONENT_HLMS=OFF ^
        -DOGRE_BUILD_COMPONENT_JAVA=OFF ^
        -DOGRE_BUILD_COMPONENT_PYTHON=OFF ^
        -DOGRE_BUILD_COMPONENT_RTSHADERSYSTEM=OFF ^
        -DOGRE_BUILD_RENDERSYSTEM_GL=OFF ^
        -DOGRE_BUILD_RENDERSYSTEM_GL3PLUS=ON ^
        -DOGRE_BUILD_RENDERSYSTEM_GLES2=OFF ^
        -DOGRE_BUILD_PLUGIN_BSP=OFF ^
        -DOGRE_BUILD_PLUGIN_OCTREE=OFF ^
        -DOGRE_BUILD_PLUGIN_PCZ=OFF ^
        -DOGRE_INSTALL_DOCS=OFF ^
        -DOGRE_INSTALL_SAMPLES=OFF ^
        -DOGRE_INSTALL_SAMPLES_SOURCE=OFF ^
        -DOGRE_INSTALL_TOOLS=OFF ^
        ..
```

Now build OGRE and install it to the local install directory specified.
```Shell
> cmake --build . --config RELEASE
> cmake --build . --target INSTALL --config RELEASE
```

### Building LLVM

The process here is essentially as described in the most recent
[LLVM documentation](https://llvm.org/docs/GettingStarted.html), so long as you
build `7.1.0` and not trunk.

Navigate to the parent directory of `open-obl` in Git Bash as when building OGRE
and clone LLVM:
```Shell
$ git clone https://github.com/llvm/llvm-project.git
$ cd llvm-project
$ git checkout llvmorg-7.1.0
```

Next, navigate to `llvm-project` in the developer prompt, make a build
directory, and configure CMake. By default LLVM will build static libraries,
though you may choose to build shared libraries by setting
`-DLLVM_BUILD_LLVM_DYLIB=ON`, provided that you set `OO_USE_SHARED_LLVM_LIBS` to
`ON` when configuring OpenOBL later.
```Shell
> mkdir Build
> cd Build
> cmake -G "Visual Studio 16 2019" ^
        -DCMAKE_BUILD_TYPE=Release ^
        -DCMAKE_INSTALL_PREFIX=../Install ^
        -DCMAKE_INSTALL_TESTS=OFF ^
        -DLLVM_BUILD_EXAMPLES=OFF ^
        -DLLVM_INCLUDE_EXAMPLES=OFF ^
        ..
```
If you run into issues building the tests, then they can be disabled by passing
`-DLLVM_BUILD_TESTS=OFF -DLLVM_INSTALL_TESTS=OFF` as additional options.

Now build and install LLVM to the local install directory:
```Shell
> cmake --build . --config RELEASE
> cmake --build . --target INSTALL --config RELEASE
```

### Building OpenOBL

With the dependencies install, navigate into `open-obl` in the developer prompt,
make a build directory, configure CMake, and build:
```Shell
> mkdir Build
> cd Build
> cmake -G "Visual Studio 16 2019" ^
        -DCMAKE_BUILD_TYPE=Release ^
        -DLLVM_ROOT=../../llvm-project/Install ^
        -DOGRE_ROOT=../../open-obl-ogre/Install ^
        ..
> cmake --build . --target OpenOBL --config RELEASE
```

## Running Oblivion with OpenOBL

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

