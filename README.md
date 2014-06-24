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

<code>
Command line flags:<br>
  -h [ --help ]            display help message<br>
  -v [ --verbose ]         enable verbose output<br>
  --version                display version information<br>
  --material_fields arg    material field paths<br>
  --background_mesh arg    input background mesh<br>
  --mesh_mode arg          background mesh mode<br>
  --mesh_improve           improve background quality<br>
  --alpha arg              initial alpha value<br>
  --sizing_field arg       sizing field path<br>
  --grading arg            sizing field grading<br>
  --multiplier arg         sizing field multiplier<br>
  --scale arg              sizing field scale<br>
  --padding arg            volume padding<br>
  --accelerate             use acceleration structure<br>
  --write_background_mesh  write background mesh<br>
  --strip_exterior         strip exterior tetrahedra<br>
  --output_path arg        output path prefix<br>
  --output_name arg        output mesh name<br>
  --output_format arg      output mesh format<br>
  --strict                 warnings become errors<br>
</code>
