# Building ITK

:::{tip}
Consider building ITK only if not already available through the system package manager.
:::

## Linux and macOS

Download ITK sources:

```bash
git clone -b v5.2.0 https://github.com/InsightSoftwareConsortium/ITK $HOME/ITK
```

Configure with:

```bash
cmake \
  -DBUILD_SHARED_LIBS=FALSE \
  -DBUILD_EXAMPLES=FALSE \
  -DBUILD_TESTING=FALSE \
  -S $HOME/ITK \
  -B $HOME/ITK-build
```

Then build ITK.

```bash
cmake --build $HOME/ITK-build --config Release --parallel 8
```

## Windows

Download ITK sources:

```bash
git clone -b v5.2.0 https://github.com/InsightSoftwareConsortium/ITK %HOMEPATH%/ITK
```

Open a Visual Studio 64 bit Native Tools Command Prompt.

Configure with:

```bash
cmake -G "NMake Makefiles" ^
  -DBUILD_SHARED_LIBS=FALSE ^
  -DBUILD_EXAMPLES=FALSE ^
  -DBUILD_TESTING=FALSE ^
  -S %HOMEPATH%/ITK ^
  -B %HOMEPATH%/ITK-build
```

Then build ITK.

```bash
cmake --build %HOMEPATH%/ITK-build --config Release --parallel 8
```

## Getting Help

See https://itk.org/ITKSoftwareGuide/html/
