include ../../Makefile.defs

TESTNAME     = gpurun-l-m
TESTSRC_MAIN = gpurun-l-m.c
TESTSRC_AUX  =
TESTSRC_ALL  = $(TESTSRC_MAIN) $(TESTSRC_AUX)

CLANG        ?= clang
OMP_BIN      = $(AOMP)/bin/$(CLANG)
CC           = $(OMP_BIN) $(VERBOSE)
RUNENV       = ulimit -s unlimited;
RUNCMD       = python3 gpurun-l-m.py

#-ccc-print-phases
#"-\#\#\#"

include ../Makefile.rules
