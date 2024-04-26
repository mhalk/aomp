include ../../Makefile.defs

TESTNAME     = spec-accel-452.ep-small
TESTSRC_MAIN = spec-accel-452.ep-small.cpp
TESTSRC_AUX  =
TESTSRC_ALL  = $(TESTSRC_MAIN) $(TESTSRC_AUX)

CLANG       ?= clang
OMP_BIN      = $(AOMP)/bin/$(CLANG)
CFLAGS       = -O3
OMP_FLAGS    = -fopenmp --offload-arch=gfx942 -fopenmp-target-fast
OMP_FLAGS   += -ffast-math -fopenmp-target-fast-reduction
# OMP_FLAGS   += -DCHECK_RESULTS
RUNENV      += OMP_TARGET_OFFLOAD=MANDATORY
CC           = $(OMP_BIN) $(VERBOSE)

include ../Makefile.rules
