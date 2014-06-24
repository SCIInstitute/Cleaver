Cleaver2
========

Cleaver2 Release Source Code

This is the open-source repository for Cleaver2, a tetrahedral meshing tool. 
This distribution comes with both a command-line interface, and a GUI.

Aknowledgements
========

TODO

Building Cleaver2
========

It is often best to build programs outside of the source tree. Inside of Cleaver2:

<code>mkdir build</code><br/>
<code>cd build</code><br/>
<code>cmake</code><br/>
<code>make</code><br/>

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
