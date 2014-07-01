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
<strong>Constributor: </strong> Ross Whitaker<br/>
<strong>Constributor: </strong> Josh Levine<br/>
<strong>Developer: </strong> Brig Bagley<br/>

Building Cleaver2
========
It is often best to build programs outside of the source tree. From Cleaver2 directory:

<h4>Linux, OSX</h4>
You will need to install prerequisites: Git, CMake, Qt5<br/>
Use git to clone the repository, then execute these commands in the Cleaver2 directory using the Command Prompt:<br/>
<code>mkdir build</code><br/>
<code>cd build</code><br/>
<code>cmake ../src</code><br/>
<code>make</code><br/>

<h4>Windows</h4>
You will need to install prerequisites: Git, CMake, glext (<link>http://sourceforge.net/projects/glextwin32/</link>), Qt5, Visual Studio 2010. <br/>
Use git to clone the repository, then execute these commands in the Cleaver2 directory using the Visual Studio 10 (32bit) Command Prompt:<br/>
<code>mkdir build</code><br/>
<code>cd build</code><br/>
<code>cmake -G "NMake Makefiles" -DGLEXT_LIBRARY="C:/glext/glext/lib/glext.lib" -DGLEXT_INCLUDE_DIR="C:/glext/glext/include" -DQt5Widgets_DIR="c:\Qt\5.3.0\5.3\msvc2010_opengl\lib\cmake\Qt5Widgets" -DQt5OpenGL_DIR="c:\Qt\5.3.0\5.3\msvc2010_opengl\lib\cmake\Qt5OpenGL"  ../src</code><br/>
<code>nmake</code><br/>


Using Cleaver2
========
Inside of your build directory:<br/>
<code>bin/cleaver-gui</code><br/>
Or, for the command line tool:<br/>
<code> bin/cleaver-cli --help</code><br/>
For a list of command line tool options.


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
