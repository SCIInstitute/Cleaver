# This variable will help provide a master list of all the sources.
# Add new source files here.
set(COIL_SOURCES
  coil.h
  coreCoil.c
  defaultsCoil.c
  enumsCoil.c
  methodsCoil.c
  realmethods.c
  scalarCoil.c
  tensorCoil.c
  )

ADD_TEEM_LIBRARY(coil ${COIL_SOURCES})
