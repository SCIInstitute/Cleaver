# Getting Started

## Introduction

Consider reading the Cleaver [Manual](manual.md).

## System requirements

* Windows 10+, macOS 10.12+, and Ubuntu 20.04 or OpenSuse 15.1+ Recommended.
* CPU: Core Duo or higher, recommended i5 or i7
* Memory: 4Gb, recommended 8Gb or more
* Dedicated Graphics Card (OpenGL 4.1+, Dedicated Shared Memory, no integrated graphics cards)
* Graphics Memory: minimum 128MB, recommended 256MB or more

:::{caution}
The following graphics cards are known to not support Cleaver:
- AMD Radeon HD 6310 (Integrated Card)
- AMD Radeon 7400 M
- INTEL HD 3000 (Integrated Card)
:::

## Using Installer

1. Download the latest [installers](https://github.com/SCIInstitute/Cleaver/releases/latest).

2. Learn about the Cleaver [Command Line Tool](manual.md#command-line-tool) and [Graphical Interface](manual.md#graphical-interface).

:::{tip}
If there is no installer available for your platform, you may consider [using python](#using-python), installing the [SlicerSegmentMesher](#using-3d-slicer) extension or building Cleaver from [source](#using-c-and-cmake).
:::


## Using Python

Cleaver is available as Python wheels distributed on PyPI for Windows, macOS and Linux for integrating in either an ITK or VTK filtering pipeline.

### ITK

1. Create a [virtual environment](https://docs.python.org/3/library/venv.html) then install the `itk-cleaver` package:

```bash
pip install itk-cleaver
```

[![PyPI](https://img.shields.io/pypi/v/itk-cleaver)](https://pypi.org/project/itk-cleaver/)

2. Generate multi-material tetrahedral mesh from a label image

```python
import itk

image = itk.imread('./mickey.nrrd')

tet_mesh, triangle_mesh = itk.cleaver_image_to_mesh_filter(image)

itk.meshwrite(triangle_mesh, './triangle_mesh.vtk')
```

### VTK

[![Project Status: WIP – Initial development is in progress, but there has not yet been a stable, usable release suitable for the public.](https://www.repostatus.org/badges/latest/wip.svg)](https://www.repostatus.org/#wip)

## Using 3D Slicer

Download [3D Slicer](https://download.slicer.org) and install the [SlicerSegmentMesher](https://github.com/lassoan/SlicerSegmentMesher#readme) extension that enables creating volumetric meshes from segmentation using Cleaver.

## Using C++ and CMake

See [Building Cleaver](project:build.md) as well as the <project:manual.md#cleaver-library> manual.
