# About

## Overview

Cleaver is an open-source multi-material tetrahedral meshing tool that creates conforming tetrahedral meshes for multimaterial or multiphase volumetric data. These meshes ensure both geometric accuracy and bounded element quality using the Lattice Cleaving algorithm.

## Method

The Cleaver Library is based on the `Lattice Cleaving` algorithm.

The method is a stencil-based approach, and relies on an octree structure to provide a coarse level of grading in regions of homogeneity. The cleaving algorithm works by utilizing indicator functions, which indicate the strength or relative presence of a particular material. At each point, only the
material with the largest indicator value is considered present.

The method is theoretically guaranteed to produce valid meshes with bounded dihedral angles, while still conforming to multimaterial material sur-
faces. Empirically these bounds have been shown to be well within useful ranges, thus creating efficient meshes for analysis, simulation, and visualization.

Reference:

> Bronson J., Levine, J., Whitaker R., "Lattice Cleaving: Conforming Tetrahedral Meshes of Multimaterial Domains with Bounded Quality". Proceedings of the 21st International Meshing Roundtable (San Jose, CA, Oct 7-10, 2012)
>
> See https://www.ncbi.nlm.nih.gov/pmc/articles/PMC4190882/

## Authors

Cleaver is an open-source project with a growing community of contributors. The software was initially developed by the NIH Center for Integrative Biomedical Computing at the University of Utah Scientific Computing and Imaging (SCI) Institute.

Many Cleaver contributors are listed in the [Contributors Graph](https://github.com/SCIInstitute/Cleaver/graphs/contributors). However, the following authors have made significant contributions to the conception, design, or implementation of the software and are considered "The Cleaver Developers":

* Jonathan Branson
* Brig Bagley
* Jess Tate
* Ally Warner
* Dan White
* Ross Whitaker

## Acknowledgement

This project was supported by the National Institute of General Medical Sciences of the National Institutes of Health under grant numbers P41 GM103545 and R24 GM136986.

## Citing Cleaver

When citing Cleaver in your scientific research, please mention the following work to support increased visibility and dissemination of our software:

> Cleaver: A MultiMaterial Tetrahedral Meshing Library and Application. Scientific Computing and Imaging Institute (SCI), Download from: http://www.sci.utah.edu/cibc/software.html, 2015.

For your convenience, you may use the following BibTex entry:

```bibtex
@Misc{SCI:Cleaver,
  author =    "CIBC",
  year =      "2015",
  note =      "Cleaver: A MultiMaterial Tetrahedral Meshing
              Library and Application. Scientific Computing and
              Imaging Institute (SCI), Download from:
              http://www.sci.utah.edu/cibc/software.html",
  keywords =  "Cleaver, CIBC",
}
```

## Bibliography

Below is a list of publications that reference Cleaver.

:::{note}
Please note that this list only includes citations from publications published after 2017 and not involving researchers or developers from the SCI Institute.
:::

```{bibliography}
:all:
```
