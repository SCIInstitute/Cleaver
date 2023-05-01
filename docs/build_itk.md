# Building ITK

:::{tip}
Consider building ITK only if not already available through the system package manager.
:::

Download sources:

```bash
git clone -b v5.2.0 https://github.com/InsightSoftwareConsortium/ITK
```

Configure with:

```bash
BUILD_SHARED_LIBS=FALSE
BUILD_EXAMPLES=FALSE
BUILD_TESTING=FALSE
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
