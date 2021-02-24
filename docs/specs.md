---
layout: default
title: Platform Specifications
category: info
tags: build
project: Cleaver2
---

## Specifications

## Minimum recommended system configuration:

+ Windows 7+, OSX 10.9+, and OpenSuse 13.1+ Recommended. Other platforms may work, but are not officially supported.
+ CPU: Core Duo or higher, recommended i5 or i7
+ Memory: 4Gb, recommended 8Gb or more
+ Dedicated Graphics Card (OpenGL 4.1+, Dedicated Shared Memory, no integrated graphics cards)
+ Graphics Memory: minimum 128MB, recommended 256MB or more

## Windows

The current source code must be compiled with the 64-bit version of Visual Studio 2015.

## Mac OS X

The source code base was built with Xcode as well as GNU Make and works for both environments on OS X 10.9+.

## Linux specifications

The code base has been tested for use with GCC, and this is the recommended compiler for linux. Compiler must support C++11.

### Build from source

{{ page.project }} can be ([compiled]({{ site.github.url }}/build.html)) from source on Linux platforms (OpenSuSE, Ubuntu etc.), OSX, and Windows. It requires at least the following:

+ C++11 64-bit compatible compiler
+ Git 1.8 or higher (https://git-scm.com/)
+ CMake 2.8+ (http://www.cmake.org/)
+ Insight Toolkit (ITK 4.7+ recommended) (http://www.itk.org/)
+ Qt 5.* (http://www.qt.io/developers/)
+ NVIDIA card and drivers for Linux
+ Graphics cards must support OpenGL 2.0 or greater (not available on older Intel embedded graphics cards).

Consult the distribution-specific section for additional package information and the developer documentation for build instructions.

#### OpenSuSE

We are currently testing on 64-bit Leap 42.1 (OpenSuSE package repository information).

OpenSuSE RPMs:

+ gcc
+ gcc-c++
+ Make
+ CMake
+ git
+ glu-devel
+ libXmu-devel
+ CMake-gui
+ CMake
