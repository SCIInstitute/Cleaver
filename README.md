Cleaver2
========

Cleaver2 Release Source Code

This is the open-source repository for Cleaver2, a tetrahedral meshing tool. 
This distribution comes with both a command-line interface, and a GUI.

Aknowledgements
========

<h4>Cleaver - A MultiMaterial Tetrahedral Meshing Library and Application</h4>

The Cleaver Library is based on the 'Lattice Cleaving' algorithm:

<strong>Bronson J., Levine, J., Whitaker R., "Lattice Cleaving: Conforming Tetrahedral Meshes of Multimaterial Domains with Bounded Quality". Proceedings of the 21st International Meshing Roundtable (San Jose, CA, Oct 7-10, 2012)</strong>

The method is theoretically guaranteed to produce valid meshes with bounded dihedral angles, while still conforming to multimaterial material surfaces. Empirically these bounds have been shown to be significant.

Cleaver is an Open Source software project that is principally funded through the SCI Institute's NIH/NIGMS CIBC Center. Please use the following acknowledgment and send us references to any publications, presentations, or successful funding applications that make use of NIH/NIGMS CIBC software or data sets.

"This project was supported by the National Institute of General Medical Sciences of the National Institutes of Health under grant number P41GM103545."

<strong>Author: </strong> Jonathan Bronson<br/>
<strong>Contributor: </strong> Ross Whitaker<br/>
<strong>Contributor: </strong> Josh Levine<br/>
<strong>Contributor: </strong> Shankar Sastry<br/>
<strong>Developer: </strong> Brig Bagley<br/>

Building Cleaver2
========
Requirements: Git, CMake, Qt4 -OR- Qt5<br/>
Suggested:  QtCreator cross-platform IDE<br/>
We recommend building cleaver outside of the source tree. <br/>
From Cleaver2 directory:<br/>

<h4>Linux, OSX</h4>

```bash
mkdir build
cd build
cmake ../src
make
```

**NOTE**: You may need to set your Qt build variables:

```bash
cmake -DQt5Widgets_DIR="/usr/lib/Qt/5.3.0/gcc/lib/cmake/Qt5Widgets" -DQt5OpenGL_DIR="/usr/lib/Qt/5.3.0/gcc/lib/cmake/Qt5OpenGL"../src 
```

<h4>Windows</h4>
Additional requirements for GUI: glew (<link>http://glew.sourceforge.net/</link>) -OR- Qt5 and glext
(<link>http://sourceforge.net/projects/glextwin32/</link>)<br/>
From Developer Command Prompt: (e.g.  Visual Studio 10 (32bit)) <br/>

```bash
mkdir build
cd build
cmake ../src
nmake
```


**NOTE**: If you do not have your development environment paths set up, you can set them with cmake-gui, qt-creator, or pass library paths directly to command line like below:<br/>

<h5>For Qt 4</h5>
```bash
cmake -G "NMake Makefiles" -DGLEW_LIBRARY="C:\glew\glew-1.10.0\lib\Release\Win32\glew32.lib" -DGLEW_INCLUDE_DIR="C:\glew\glew-1.10.0\include" -DQT_QMAKE_EXECUTABLE="C:\Qt\4.8.5\bin\qmake.exe" -DQT_VERSION="4" ..\src
```
<h5>For Qt 5</h5>
```bash
cmake -G "NMake Makefiles" -DGLEXT_LIBRARY="C:\glext\glext\lib\glext.lib" -DGLEXT_INCLUDE_DIR="C:\glext\glext\include" -DQt5Widgets_DIR="c:\Qt\5.3.0\5.3\msvc2010_opengl\lib\cmake\Qt5Widgets" -DQt5OpenGL_DIR="c:\Qt\5.3.0\5.3\msvc2010_opengl\lib\cmake\Qt5OpenGL"  -DQT_VERSION="5" ..\src
```

Using Cleaver2
========
Inside of your build directory:<br/>
<code>bin/cleaver-gui</code><br/>
Or, for the command line tool:<br/>
<code> bin/cleaver-cli --help</code><br/>
For a list of command line tool options.

```bash
    Command line flags:
      -h [ --help ]            display help message
      -v [ --verbose ]         enable verbose output
      --version                display version information
      --material_fields arg    material field paths
      --background_mesh arg    input background mesh
      --mesh_mode arg          background mesh mode
      --mesh_improve           improve background quality
      --alpha arg              initial alpha value
      --alpha_long arg         alpha long value for regular mesh_mode
      --alpha_short arg        alpha short value for regular mesh_mode
      --sizing_field arg       sizing field path
      --grading arg            sizing field grading
      --multiplier arg         sizing field multiplier
      --scale arg              sizing field scale
      --padding arg            volume padding
      --accelerate             use acceleration structure
      --write_background_mesh  write background mesh
      --strip_exterior         strip exterior tetrahedra
      --output_path arg        output path prefix
      --output_name arg        output mesh name
      --output_format arg      output mesh format
      --strict                 warnings become errors
```

Testing
==============
The repo comes with a set of regression tests to see if recent changes break expected results. To build the tests, you will need to set <code>BUILD_TESTING</code> to "ON" in either <code>ccmake</code> or when calling CMake:

```c++
cmake -DBUILD_TESTING=ON ../src
```
<h4>Windows</h4>
The gtest library included in the repo needs to be built with forced shared libraries on Windows, so use the following:

```c++
cmake -DBUILD_TESTING=ON -Dgtest_forced_shared_crt=ON ../src
```
Be sure to include all other necessary CMake definitions as annotated above.

Known Issues
========

 * On larger data sets with a potentially high number of quadruple points (> 3 material fields), some functions are failing to ensure valid tets and meshes, causing bad tets in the final output. This code is being debugged now for a future release.
