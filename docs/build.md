# Building

```{toctree}
:hidden:

build_itk.md
```

## Overview

Building Cleaver is the process of obtaining a copy of the source code of the project and use tools, such as compilers, project generators and build systems, to create binary libraries and executables.

Cleaver can be compiled from source on Linux platforms (OpenSuSE, Ubuntu etc.), macOS, and Windows.

CMake is a cross-platform build system generator that is used for generating Makefiles, Ninja, Visual Studio or Xcode project files.

The build-system generator provides options to selectively build Cleaver [Command Line Tool](manual.md#command-line-tool) and [Graphical Interface](manual.md#graphical-interface).

:::{tip}
Users of the Cleaver command line tool or graphical interface may not need to build the project as they can instead download and install pre-built packages as described in the <project:getting_started.md> section.
:::

## Build Environment

**Windows**: 64-bit version of Visual Studio 2015 or newer.

**macOS**: macOS 10.12+ using either `Ninja`, `Xcode` or `Unix Makefiles` CMake generator.

**Linux**: GCC compiler supporting C++11 using either `Ninja` or `Unix Makefiles` CMake generator.

## Build Options

The table below describes some of build options available when configuring Cleaver using CMake.

| Option      | Description | Default |
|-------------|-------------|---------|
| `BUILD_CLI` | Build Cleaver Command Line Tool (CLI) application   | `OFF` |
| `BUILD_GUI` | Build Cleaver Graphical Interface (GUI) application | `OFF` |

:::{tip}
By default, when both `BUILD_CLI` and `BUILD_GUI` are `OFF`, only the <project:manual.md#cleaver-library> is built.
:::

## Dependencies

### Tools

+ C++11 64-bit compatible compiler
* [Git](https://git-scm.com/) 1.8 or higher
* [CMake](https://www.cmake.org/) 3.10.2+

### Libraries

|       | Command Line Tool                 | Graphical Interface               |
|-------|-----------------------------------|-----------------------------------|
| ITK   | {octicon}`check;1em;sd-text-info` | {octicon}`check;1em;sd-text-info` |
| Qt    |                                   | {octicon}`check;1em;sd-text-info` |


**Qt**:

Qt 5 libraries are required to build the Cleaver Graphical Interface. The libraries may be installed by downloading the [Qt universal installer](https://www.qt.io/download-open-source) and selecting the Qt 5.15.2 components.

Alternatively, the Qt libraries may be installed through the system package manager or as a least resort by building Qt from source.

**ITK**:

[ITK](http://www.itk.org/) Insight Toolkit 5.0+ is required. See  <project:build_itk.md>.


## Building

Once CMake, Qt, ITK have been installed and/or built:
1. Download the Cleaver sources.
2. Run CMake to configure the project by specifying a build directory, the path to the Cleaver directory containing the `src/CMakeLists.txt` file.
3. Build the project

### Linux and macOS

```bash
git clone https://github.com/SCIInstitute/Cleaver.git $HOME/Cleaver

cmake \
  -DITK_DIR:PATH=$HOME/ITK-build \
  -DQt5_DIR:PATH=/Path/To/Qt5/lib/cmake/Qt5 \
  -DCMAKE_BUILD_TYPE:STRING=Release \
  -DBUILD_CLI:BOOL=ON \
  -DBUILD_GUI:BOOL=ON \
  -S $HOME/Cleaver/src \
  -B $HOME/Cleaver-build

cmake --build $HOME/Cleaver-build --config Release --parallel 8
```

### Windows

Open a Visual Studio 64 bit Native Tools Command Prompt.

Follow these commands:

```
git clone https://github.com/SCIInstitute/Cleaver.git %HOMEPATH%/Cleaver

set Qt5_DIR=C:/Path/To/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5

cmake -G "NMake Makefiles" ^
  -DQt5_DIR:PATH="%Qt5_DIR%" ^
  -DITK_DIR:PATH="%HOMEPATH%/ITK-build" ^
  -DCMAKE_BUILD_TYPE:STRING=Release ^
  -DBUILD_CLI:BOOL=ON ^
  -DBUILD_GUI:BOOL=ON ^
  -S %HOMEPATH%/Cleaver/src ^
  -B %HOMEPATH%/Cleaver-build

cmake --build %HOMEPATH%/Cleaver-build --config Release --parallel 8
```

:::{warning}
Be sure to copy the Qt5 DLL files to the Executable directory for the program to run.

```
copy %Qt5_DIR%\..\..\..\bin\Qt5Core.dll %HOMEPATH%\Cleaver-build\bin\
copy %Qt5_DIR%\..\..\..\bin\Qt5Gui.dll %HOMEPATH%\Cleaver-build\bin\
copy %Qt5_DIR%\..\..\..\bin\Qt5OpenGL.dll %HOMEPATH%\Cleaver-build\bin\
copy %Qt5_DIR%\..\..\..\bin\Qt5Widgets.dll %HOMEPATH%\Cleaver-build\bin\
```
:::

### All Platforms

Your paths may differ slightly based on your Qt5 and ITK versions and where they are installed/built.

The console version `ccmake`, or GUI version can also be used. You may be prompted to specify your location of the Qt installation. If you installed Qt in the default location, it should find Qt automatically. After configuration is done, generate the make files or project files for your favorite development environment and build.

The Cleaver applications will be built in `Cleaver-build/bin`.

## Testing

The repo comes with a set of regression tests to see if recent
changes break expected results.

### Linux and macOS

To build the tests, you may set `BUILD_TESTING` to `ON` in using either `ccmake` or when calling CMake:

```bash
cmake -DBUILD_TESTING=ON $HOME/Cleaver/src
```

## Windows

The gtest library included in the repo needs to be
built with forced shared libraries on Windows, so use the following:

```bash
cmake -DBUILD_TESTING=ON -Dgtest_forced_shared_crt=ON %HOMEPATH%/Cleaver/src
```
Be sure to include all other necessary CMake definitions as annotated above.

