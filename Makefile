CPPFLAGS = -I dmklib
CFLAGS = -O -Wall -g

clean_files = fgrdmk

all: fgrdmk

fgrdmk: fgrdmk.c dmklib/libdmk.o

dmklib/libdmk.o: dmklib/libdmk.c dmklib/libdmk.h
	$(MAKE) -C dmklib

clean:
	rm -f -- $(wildcard $($@_files))
	$(MAKE) -C dmklib $@


.PHONY: all clean
.DELETE_ON_ERROR:
