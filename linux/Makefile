# Unix makefile for C eXtensions Library (CXL).		Ver. 1.0.0

# Definitions.
LibName = libcx.a
SrcDir = ../src
ObjDir = obj
InclDir = ../include/cxl

# List of object files.
ObjFiles =\
 $(ObjDir)/array.o\
 $(ObjDir)/datum.o\
 $(ObjDir)/excep.o\
 $(ObjDir)/fastio.o\
 $(ObjDir)/fviz.o\
 $(ObjDir)/getDelim.o\
 $(ObjDir)/getSwitch.o\
 $(ObjDir)/hash.o\
 $(ObjDir)/intf.o\
 $(ObjDir)/join.o\
 $(ObjDir)/memcasecmp.o\
 $(ObjDir)/memstpcpy.o\
 $(ObjDir)/prime.o\
 $(ObjDir)/split.o\
 $(ObjDir)/stplcpy.o\
 $(ObjDir)/strconv.o\
 $(ObjDir)/strfit.o\
 $(ObjDir)/strip.o\
 $(ObjDir)/strpspn.o\
 $(ObjDir)/strtoint.o\
 $(ObjDir)/version.o\
 $(ObjDir)/vizc.o

InclFlags = -I../../include -I../include

# Options and arguments to the C compiler.
CC = cc
CFLAGS = -std=c99 -funsigned-char -W -Wall -Wunused\
 -Wno-comment -Wno-missing-field-initializers -Wno-missing-braces -Wno-pointer-sign -Wno-unused-parameter -O2 $(InclFlags)

# Targets.
.PHONY: all clean

all: $(ObjDir) $(LibName)

$(ObjDir):
	@if [ ! -d $@ ]; then \
		mkdir $@;\
		echo "Created directory '$@'." 1>&2;\
	fi

$(LibName): $(ObjFiles)
	-[ -f $(LibName) ] && rm $(LibName)
	@if [ "`uname -s`" = Darwin ]; then \
		echo 'libtool -static -o $(LibName) $(ObjFiles)' 1>&2;\
		libtool -static -o $@ $(ObjFiles);\
	else \
		echo 'ar r $@ $(ObjFiles)' 1>&2;\
		ar r $@ $(ObjFiles);\
		echo 'ranlib $@' 1>&2;\
		ranlib $@;\
	fi

$(ObjDir)/array.o: $(SrcDir)/array.c $(InclDir)/excep.h $(InclDir)/datum.h $(InclDir)/array.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/array.c

$(ObjDir)/datum.o: $(SrcDir)/datum.c $(InclDir)/excep.h $(InclDir)/datum.h $(InclDir)/string.h $(InclDir)/ioext.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/datum.c

$(ObjDir)/excep.o: $(SrcDir)/excep.c $(InclDir)/excep.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/excep.c

$(ObjDir)/fastio.o: $(SrcDir)/fastio.c $(InclDir)/excep.h $(InclDir)/string.h $(InclDir)/fastio.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/fastio.c

$(ObjDir)/fviz.o: $(SrcDir)/fviz.c $(InclDir)/excep.h $(InclDir)/string.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/fviz.c

$(ObjDir)/getDelim.o: $(SrcDir)/getDelim.c $(InclDir)/excep.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/getDelim.c

$(ObjDir)/getSwitch.o: $(SrcDir)/getSwitch.c $(InclDir)/excep.h $(InclDir)/lib.h $(InclDir)/string.h $(InclDir)/getSwitch.h\
 $(InclDir)/hash.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/getSwitch.c

$(ObjDir)/hash.o: $(SrcDir)/hash.c $(InclDir)/excep.h $(InclDir)/lib.h $(InclDir)/hash.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/hash.c

$(ObjDir)/intf.o: $(SrcDir)/intf.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/intf.c

$(ObjDir)/join.o: $(SrcDir)/join.c $(InclDir)/datum.h $(InclDir)/string.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/join.c

$(ObjDir)/memcasecmp.o: $(SrcDir)/memcasecmp.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/memcasecmp.c

$(ObjDir)/memstpcpy.o: $(SrcDir)/memstpcpy.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/memstpcpy.c

$(ObjDir)/prime.o: $(SrcDir)/prime.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/prime.c

$(ObjDir)/split.o: $(SrcDir)/split.c $(InclDir)/excep.h $(InclDir)/string.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/split.c

$(ObjDir)/stplcpy.o: $(SrcDir)/stplcpy.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/stplcpy.c

$(ObjDir)/strconv.o: $(SrcDir)/strconv.c $(InclDir)/excep.h $(InclDir)/datum.h $(InclDir)/string.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/strconv.c

$(ObjDir)/strfit.o: $(SrcDir)/strfit.c $(InclDir)/string.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/strfit.c

$(ObjDir)/strip.o: $(SrcDir)/strip.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/strip.c

$(ObjDir)/strpspn.o: $(SrcDir)/strpspn.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/strpspn.c

$(ObjDir)/strtoint.o: $(SrcDir)/strtoint.c $(InclDir)/excep.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/strtoint.c

$(ObjDir)/version.o: $(SrcDir)/version.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/version.c

$(ObjDir)/vizc.o: $(SrcDir)/vizc.c $(InclDir)/excep.h $(InclDir)/string.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/vizc.c

clean:
	@rm -f $(LibName) $(ObjDir)/*.o
