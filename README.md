Cleaver2
========

Cleaver2 Source Code

This is the open-source repository for Cleaver2, a tetrahedral meshing tool. 
This distribution comes with both a command-line interface, and a GUI.

<h3>Aknowledgements</h3>

**Cleaver - A MultiMaterial Tetrahedral Meshing Library and Application**

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
Optional Requirements: SCIRun4 (For segmentation tools)
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

**NOTE**: To include Segmentation Tools in your build, you must set your SCIRun4 directory:<br/>
```bash
cmake -DSCIRun4_DIR="/Path/To/SCIRun" ../src
```
<br/>
**NOTE**: Since the segmentation tools make system calls, it is important that you provide the full path to the segmentation file (not a relative path).<br/>

Using Cleaver2
========
<h3>Command line Tool:</h3>
<code> bin/cleaver-cli --help</code><br/>
For a list of command line tool options.

```bash
Command line flags:
  -h [ --help ]                   display help message
  -v [ --verbose ]                enable verbose output
  --version                       display version information
  -i [ --material_fields ] arg    material field paths
  -b [ --background_mesh ] arg    input background mesh
  -m [ --mesh_mode ] arg          background mesh mode (structured [default], 
                                  regular)
  -a [ --alpha ] arg              initial alpha value
  -s [ --alpha_short ] arg        alpha short value for regular mesh_mode
  -l [ --alpha_long ] arg         alpha long value for regular mesh_mode
  -z [ --sizing_field ] arg       sizing field path
  -g [ --grading ] arg            sizing field grading
  -x [ --multiplier ] arg         sizing field multiplier
  -c [ --scale ] arg              sizing field scale
  -p [ --padding ] arg            volume padding
  -w [ --write_background_mesh ]  write background mesh
  -j [ --fix_tet_windup ]         Ensure positive Jacobians with proper vertex 
                                  wind-up.
  -e [ --strip_exterior ]         strip exterior tetrahedra
  -o [ --output_path ] arg        output path prefix
  -n [ --output_name ] arg        output mesh name [default 'output']
  -f [ --output_format ] arg      output mesh format (tetgen [default], scirun,
                                  matlab, vtk)
  -t [ --strict ]                 warnings become errors
```
<h3>Graphical Interface</h3>
You can run the GUI from the command line, or by double-clicking it in a folder.
<code> bin/cleaver-gui</code><br/>
You should see a window similar to this:<br/>
<img src="https://raw.githubusercontent.com/SCIInstitute/Cleaver2/master/src/gui/Resources/application.png"><br/>
**Sizing Field Creator**<br/>
This tool allows a user to set parameters for the cleaving sizing field.<br/>
*Volume* If you have loaded multiple volumes, you can switch between them here for the sizing field.<br/>
*Size Multiplier* This is the multiplier for the sizing field creation.<br/>
*Sample Scale* This is a tool to alter the sampling size of a volume. 
Smaller sampling creates coarser meshes faster.<br/>
*Lipschitz* This is the grading of the sizing field. <br/>
*Padding* Added a volume buffer around the data. This is useful when volumes intersect near the boundary.<br/>
*Surface Size* Select whether to adaptively resize tets for more detail at volume interactions, or 
to keep tet sizes constant based on the sample scale.<br/>
*Compute Sizing Field* Once you have your desired parameters, click this to create the sizing field. 
This is assuming a volume has been loaded (ctrl+v or File->Import Volume). New information will be added 
to the Data Manager at each step. If a sizing field is not created here, a default one will be 
created for you automatically before cleaving. <br/>
<img src="https://raw.githubusercontent.com/SCIInstitute/Cleaver2/master/src/gui/Resources/mesh.png"><br/>
**Cleaving Tool : Adaptive**<br/>
This tab runs the new adaptive technique of cleaving. Smaller tets occur near volume interactions 
for more detail.<br/>
*Volume* If you have loaded multiple volumes, you can switch between them here for cleaving.<br/>
*Mesh* This feature is not currently available.<br/>
*Octree Background Mesh* Check this to allow for octree creation of the background mesh.<br/>
*Reorder inverted elements* Check this to ensure tets (after cleaving) have the proper vertex order.<br/>
*Cleave Mesh* Run the cleaving algorithm. The steps are shown as complete with the check below. 
The rendering window will also update with each step.<br/>
**Cleaving Tool : Regular**<br/>
This tab is for cleaving in a manner similar to Cleaver1. The sizing field is constant.<br/>
*Volume* If you have loaded multiple volumes, you can switch between them here for cleaving.<br/>
*Alpha Short* This is the distance (as a percent/100) of a short edge along a tet for which violations
will be considered for warping.<br/>
*Alpha Long* This is the distance (as a percent/100) of a long edge along a tet for which violations
will be considered for warping.<br/>
*Sample Scale* This is a tool to alter the sampling size of a volume. 
Smaller sampling creates coarser meshes faster.<br/>
*Padding* Added a volume buffer around the data. This is useful when volumes intersect near the boundary.<br/>
*Construct Mesh* Run the cleaving algorithm. The steps are shown as complete with the check below. 
The rendering window will also update with each step.<br/>
**Data Manager**<br/>
This tool displays information about meshes, volumes, and sizing fields loaded and created. <br/>
*Mesh* A mesh will have number of vertices, number of tetrahedra, and the min/max mesh boundaries.<br/>
*Volume* A volume will display the dimensions, origin, number of materials, the file names,
and the associated sizing field (if any other than the default).<br/>
**Mesh View Options**<br/>
Here are are a number of options for visualizing the generated mesh.<br/>
<img src="https://raw.githubusercontent.com/SCIInstitute/Cleaver2/master/src/gui/Resources/surface.png"><br/>
*Show Axis* Toggle the rendering of the coordinate axis (x-y-z) <br/>
*Show BBox* Toggle the rendering of the mesh/volume bounding box. <br/>
*Show Mesh Faces* Toggle the rendering of the mesh's faces. <br/>
*Show Edges* Toggle the rendering of the mesh's edges. <br/>
*Show Cuts* Toggle the rendering of nodes where cuts took place. <br/>
*Show Surfaces Only* Toggle the rendering of the tets (volume) vs. the surface 
representing the interface between volumes. <br/>
*Color by Quality* Toggle the coloring of faces based on the quality of the tet vs. the material itself. <br/>
<img src="https://raw.githubusercontent.com/SCIInstitute/Cleaver2/master/src/gui/Resources/clip.png"><br/>
*Clipping* Toggle the clipping of tets based on the below sliders. <br/>
*Sync* When checked, faces will update during slider movement (slower). Otherwise, 
faces will update once the clipping plane has stopped moving (mouse is released). <br/>
*X-Y-Z Axes* Select which axis to clip the volume. The associated slider will permit clipping
from one end of the bounding box to the other. <br/>
*Material Visibility Locks* A list of the materials is here. When the faces of a material is locked, clipping
is ignored for that material and it is always visible. Locked cells currently has no affect<br/>
**File Menu**<br/>
*Import Volume* Select 1-10 indicator function NRRDs, or 1 segmentation NRRD (if built in) to load in.<br/>
*Import Sizing Field* Load a sizing field NRRD to use for a Volume.<br/>
*Import Mesh* Import a tetgen mesh (*.node/*.ele pair) to visualize.<br/>
*Export Mesh* Write the current mesh to file in either node/ele (tetgen) format, or VTK format. <br/>
*Close* Close the current mesh rendering. (Known to have issues). <br/>
*Close All* Close all the mesh renderings. (Known to have issues). <br/>
**Edit Menu**<br/>
*Remove External Tets* Removes tets that were created as padding around the volume.<br/>
*Remove Locked Tets* Removes tets that were not warped during cleaving.<br/>
**View** Reset, Load, and Save the current camera view of the mesh. <br/>
**Tools** Toggle view of the Sizing Field, Cleaving, Data, and Mesh View tools. <br/> 
**Window** Select which render window to view. (Known to have issues). <br/>

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
