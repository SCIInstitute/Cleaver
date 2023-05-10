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
2. Create a build directory
3. Run CMake from your build directory and give a path to the Cleaver directory containing the `src/CMakeLists.txt` file.

### Linux and macOS

```bash
git clone https://github.com/SCIInstitute/Cleaver.git

mkdir Cleaver/build

cd Cleaver/build

cmake \
  -DITK_DIR:PATH=$HOME/ITK-build \
  -DQT_DIR:PATH=/Path/To/Your/Qt5/build \
  -DCMAKE_BUILD_TYPE:STRING=Release \
  ../src

make
```

:::{warning}
Depending on how you obtained Qt, you may need to specify other Qt directories:
```bash
-DQt5Widgets_DIR:PATH="/Path/To/Qt/5.6/gcc/lib/cmake/Qt5Widgets"
-DQt5OpenGL_DIR:PATH="/Path/To/Qt/5.6/gcc/lib/cmake/Qt5OpenGL"
```
:::

### Windows

Open a Visual Studio 64 bit Native Tools Command Prompt.

Follow these commands:

```
mkdir C:\Path\To\Cleaver\build

cd C:\Path\To\Cleaver\build

cmake -G "NMake Makefiles" ^
  -DITK_DIR:PATH="%HOMEPATH%/ITK-build" ^
  -DQT_DIR:PATH="C:/Path/To/Your/Qt5/build" ^
  -DCMAKE_BUILD_TYPE:STRING=Release ^
  ../src

nmake
```

:::{warning}
Be sure to copy the Qt5 DLL files to the Executable directory for the program to run.
```
C:\Qt5_DIR\msvc2015\5.6\bin\Qt5Widgets.dll
C:\Qt5_DIR\msvc2015\5.6\bin\Qt5Core.dll
C:\Qt5_DIR\msvc2015\5.6\bin\Qt5OpenGL.dll
C:\Qt5_DIR\msvc2015\5.6\bin\Qt5Gui.dll
```
:::

### All Platforms

Your paths may differ slightly based on your Qt5 and ITK versions and where they are installed/built.

The console version `ccmake`, or GUI version can also be used. You may be prompted to specify your location of the Qt installation. If you installed Qt in the default location, it should find Qt automatically. After configuration is done, generate the make files or project files for your favorite development environment and build.

The Cleaver applications will be built in `build/bin`.

## Testing

The repo comes with a set of regression tests to see if recent
changes break expected results.

To build the tests, you may set `BUILD_TESTING` to `ON` in using either `ccmake` or when calling CMake:

```bash
cmake -DBUILD_TESTING=ON ../src
```

## Windows

The gtest library included in the repo needs to be
built with forced shared libraries on Windows, so use the following:

```bash
cmake -DBUILD_TESTING=ON -Dgtest_forced_shared_crt=ON ../src
```
Be sure to include all other necessary CMake definitions as annotated above.

