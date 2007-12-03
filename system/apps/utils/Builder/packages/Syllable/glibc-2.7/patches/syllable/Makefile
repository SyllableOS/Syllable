# Stub Makefile to keep Makerules happy; the ports root directory ends up in $(sysd-sorted) but
# we have nothing to build here, so make falls over when it can't find targets it was expecting

subdir := syllable

include ../Makeconfig

generated += stamp.o

ifeq (yes,$(build-shared))
generated += stamp.os
else
generated += stamp.oS
endif

include ../Rules

.PHONY: subdir_lib lib other install

subdir_lib:
	touch $(objpfx)stamp.o $(objpfx)stamp.os $(objpfx)stamp.oS

lib:

other:

install:

