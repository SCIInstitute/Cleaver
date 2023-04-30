# Installing Cleaver from source

## Dependencies

### Qt

[Qt binaries](qt.io) and packages are available on the Qt website or can be built 
from source code. Gcc/Clang/MSVC with C++11 support is required.

### CMake

[CMake](https://cmake.org/) versions 2.8 - 3.4 are supported.

### ITK

[ITK](http://www.itk.org/) Insight Toolkit (ITK 4.7+ recommended) 


## Compiling From Source

Once you have obtained a compatible compiler and installed Qt on your system, you need to
download and install CMake (<http://www.cmake.org>) to actually build the software.
CMake is a platform independent configuring system that is used for generating Makefiles,Visual Studio project files, or Xcode project files.

### Compiling ITK

Configure with:
```c++
CMAKE_CXX_FLAGS+="-std=c++11"
BUILD_SHARED_LIBS=FALSE
BUILD_EXAMPLES=FALSE
BUILD_TESTING=FALSE
ITKV3_COMPATIBILTY=TRUE 
```

Then build ITK.

```bash 
make -j12 all 
```

You may need to use the CMake GUI in Windows. It is best to configure with `NMake Makefiles`. Once you have configured and generated, you can build in a command prompt.

```bash
cd C:\ITK_DIR
mkdir build
cd build
nmake all
```

### Compiling Cleaver
Once CMake, Qt, ITK have been installed and/or built, run CMake from your build directory and give a path to the ShapeworksStudio directory containing the master CMakeLists.txt file.

#### Unix and OSX
```bash
mkdir Cleaver/build
cd Cleaver/build
cmake -D ITK_DIR=Path/To/Your/ITK/build -D QT_DIR=Path/To/Your/Qt5/build -D CMAKE_BUILD_TYPE=Release ../src
make
```
Depending on how you obtained Qt, you may need to specify other Qt directories:
```bash
-D Qt5Widgets_DIR="Path/To/Qt/5.6/gcc/lib/cmake/Qt5Widgets"
-D Qt5OpenGL_DIR="Path/To/Qt/5.6/gcc/lib/cmake/Qt5OpenGL"
```

#### Windows
Open a Visual Studio 64 bit Native Tools Command Prompt.
Follow these commands:
```
mkdir C:\Path\To\Cleaver\build
cd C:\Path\To\Cleaver\build
cmake -G "NMake Makefiles" -DITK_DIR="C:/Path/To/Your/ITK/build" -DQT_DIR="C:/Path/To/Your/Qt5/build" -DCMAKE_BUILD_TYPE=Release ../src
nmake
```
**NOTE** Be sure to copy the Qt5 DLL files to the Executable directory for the program to run.
```
C:\Qt5_DIR\msvc2015\5.6\bin\Qt5Widgets.dll
C:\Qt5_DIR\msvc2015\5.6\bin\Qt5Core.dll
C:\Qt5_DIR\msvc2015\5.6\bin\Qt5OpenGL.dll
C:\Qt5_DIR\msvc2015\5.6\bin\Qt5Gui.dll
```

#### All Platforms
Your paths may differ slightly based on your Qt5 and ITK versions and where they are installed/built.

The console version `ccmake`, or GUI version can also be used. You may be prompted to specify your location of the Qt installation. If you installed Qt in the default location, it should find Qt automatically. After configuration is done, generate the make files or project files for your favorite development environment and build.

The Cleaver application will be built in build/bin.

## Testing

The repo comes with a set of regression tests to see if recent
changes break expected results. To build the tests, you will
need to set `BUILD_TESTING` to `ON` in either
`ccmake` or when calling CMake:

```c++
cmake -DBUILD_TESTING=ON ../src
```

## Windows
The gtest library included in the repo needs to be
built with forced shared libraries on Windows, so use the following:

```c++
cmake -DBUILD_TESTING=ON -Dgtest_forced_shared_crt=ON ../src
```


Be sure to include all other necessary CMake definitions as annotated above.

# Cleaver Support

For questions and issues regarding building the software from source,
    please email our support list <cleaver@sci.utah.edu>
