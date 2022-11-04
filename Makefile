# Makefile for CXL library.		Ver. 1.2.0

# Definitions.
MAKEFLAGS = --no-print-directory
ProjName = CXL
LibName = libcx.a
SrcDir = src
ObjDir = obj
InclDir = include
InclSubdir = cxl
InclPath = $(InclDir)/$(InclSubdir)
InclFlags = -I$(InclDir)
LibDir = lib
HdrFileGlob = {$(InclSubdir),stdos.h}
Install1 = /usr/local
ManDir = share/man
SrcManDir = man

# Options and arguments to the C compiler.
CC = cc
CFLAGS = -std=c99 -funsigned-char -W -Wall -Wextra -Wunused\
 -Wno-comment -Wno-missing-field-initializers -Wno-missing-braces -Wno-parentheses\
 -Wno-pointer-sign -Wno-unused-parameter $(COPTS)\
 -O2 $(InclFlags)

# List of object files.
ObjFiles =\
 $(ObjDir)/array.o\
 $(ObjDir)/bmsearch.o\
 $(ObjDir)/convDelim.o\
 $(ObjDir)/datum.o\
 $(ObjDir)/excep.o\
 $(ObjDir)/fastio.o\
 $(ObjDir)/fviz.o\
 $(ObjDir)/getSwitch.o\
 $(ObjDir)/hash.o\
 $(ObjDir)/intf.o\
 $(ObjDir)/join.o\
 $(ObjDir)/memcasecmp.o\
 $(ObjDir)/memstpcpy.o\
 $(ObjDir)/prime.o\
 $(ObjDir)/rand32.o\
 $(ObjDir)/split.o\
 $(ObjDir)/stplcpy.o\
 $(ObjDir)/stplvizcpy.o\
 $(ObjDir)/strcbrk.o\
 $(ObjDir)/strconv.o\
 $(ObjDir)/strfit.o\
 $(ObjDir)/strip.o\
 $(ObjDir)/strrev.o\
 $(ObjDir)/strtoint.o\
 $(ObjDir)/version.o\
 $(ObjDir)/vizc.o

# Targets.
.PHONY: all build-msg uninstall install user-install clean

all: build-msg $(LibName)

build-msg:
	@if [ ! -f $(LibName) ]; then \
		echo "Building $(ProjName) library..." 1>&2;\
	fi

$(LibName): $(ObjDir) $(ObjFiles)
	@if [ -f $(LibName) ]; then \
		rm $(LibName);\
	fi;\
	if [ "`uname -s`" = Darwin ]; then \
		echo 'libtool -static -o $(LibName) $(ObjFiles)' 1>&2;\
		libtool -static -o $(LibName) $(ObjFiles);\
	else \
		echo 'ar r $(LibName) $(ObjFiles)' 1>&2;\
		ar r $(LibName) $(ObjFiles);\
		echo 'ranlib $(LibName)' 1>&2;\
		ranlib $(LibName);\
	fi

$(ObjDir):
	@if [ ! -d $@ ]; then \
		mkdir $@ && echo "Created directory '$@'." 1>&2;\
	fi

$(ObjDir)/array.o: $(SrcDir)/array.c $(InclPath)/excep.h $(InclPath)/datum.h $(InclPath)/array.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/array.c
$(ObjDir)/bmsearch.o: $(SrcDir)/bmsearch.c $(InclPath)/excep.h $(InclPath)/bmsearch.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/bmsearch.c
$(ObjDir)/convDelim.o: $(SrcDir)/convDelim.c $(InclPath)/excep.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/convDelim.c
$(ObjDir)/datum.o: $(SrcDir)/datum.c $(InclPath)/excep.h $(InclPath)/datum.h $(InclPath)/string.h $(InclPath)/ioext.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/datum.c
$(ObjDir)/excep.o: $(SrcDir)/excep.c $(InclPath)/excep.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/excep.c
$(ObjDir)/fastio.o: $(SrcDir)/fastio.c $(InclPath)/excep.h $(InclPath)/string.h $(InclPath)/fastio.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/fastio.c
$(ObjDir)/fviz.o: $(SrcDir)/fviz.c $(InclPath)/excep.h $(InclPath)/string.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/fviz.c
$(ObjDir)/getSwitch.o: $(SrcDir)/getSwitch.c $(InclPath)/excep.h $(InclPath)/lib.h $(InclPath)/string.h $(InclPath)/getSwitch.h\
 $(InclPath)/hash.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/getSwitch.c
$(ObjDir)/hash.o: $(SrcDir)/hash.c $(InclPath)/excep.h $(InclPath)/lib.h $(InclPath)/hash.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/hash.c
$(ObjDir)/intf.o: $(SrcDir)/intf.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/intf.c
$(ObjDir)/join.o: $(SrcDir)/join.c $(InclPath)/datum.h $(InclPath)/string.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/join.c
$(ObjDir)/memcasecmp.o: $(SrcDir)/memcasecmp.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/memcasecmp.c
$(ObjDir)/memstpcpy.o: $(SrcDir)/memstpcpy.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/memstpcpy.c
$(ObjDir)/prime.o: $(SrcDir)/prime.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/prime.c
$(ObjDir)/rand32.o: $(SrcDir)/rand32.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/rand32.c
$(ObjDir)/split.o: $(SrcDir)/split.c $(InclPath)/excep.h $(InclPath)/string.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/split.c
$(ObjDir)/stplcpy.o: $(SrcDir)/stplcpy.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/stplcpy.c
$(ObjDir)/stplvizcpy.o: $(SrcDir)/stplvizcpy.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/stplvizcpy.c
$(ObjDir)/strcbrk.o: $(SrcDir)/strcbrk.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/strcbrk.c
$(ObjDir)/strconv.o: $(SrcDir)/strconv.c $(InclPath)/excep.h $(InclPath)/string.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/strconv.c
$(ObjDir)/strfit.o: $(SrcDir)/strfit.c $(InclPath)/string.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/strfit.c
$(ObjDir)/strip.o: $(SrcDir)/strip.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/strip.c
$(ObjDir)/strrev.o: $(SrcDir)/strrev.c
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/strrev.c
$(ObjDir)/strtoint.o: $(SrcDir)/strtoint.c $(InclPath)/excep.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/strtoint.c
$(ObjDir)/version.o: $(SrcDir)/version.c $(InclPath)/lib.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/version.c
$(ObjDir)/vizc.o: $(SrcDir)/vizc.c $(InclPath)/excep.h $(InclPath)/string.h
	$(CC) -c -o $@ $(CFLAGS) $(SrcDir)/vizc.c

uninstall:
	@echo 'Uninstalling...' 1>&2;\
	if [ -n "$(INSTALL)" ] && [ -f "$(INSTALL)/$(LibDir)/$(LibName)" ]; then \
		destDir="$(INSTALL)";\
	elif [ -f "$(Install1)/$(LibDir)/$(LibName)" ]; then \
		destDir=$(Install1);\
	else \
		destDir=;\
	fi;\
	if [ -z "$$destDir" ]; then \
		echo "'$(LibName)' library not found." 1>&2;\
	else \
		for f in "$$destDir"/$(LibDir)/$(LibName) "$$destDir"/include/$(HdrFileGlob); do \
			if [ -e "$$f" ]; then \
				rm -rf "$$f" && echo "Deleted '$$f'" 1>&2;\
			fi;\
		done;\
		cd $(SrcManDir) || exit $$?;\
		for n in 3 7; do \
			for x in *.$$n; do \
				f="$$destDir"/$(ManDir)/man$$n/$$x;\
				rm -f "$$f" 2>/dev/null && echo "Deleted '$$f'" 1>&2;\
			done;\
		done;\
	fi;\
	echo 'Done.' 1>&2

install: uninstall
	@echo 'Beginning installation...' 1>&2;\
	umask 022;\
	myID=`id -u`;\
	comp='-C' permCheck=true destDir=$(INSTALL);\
	if [ -z "$$destDir" ]; then \
		destDir=$(Install1);\
	else \
		[ $$myID -ne 0 ] && permCheck=false;\
		if [ ! -d "$$destDir" ]; then \
			mkdir "$$destDir" || exit $$?;\
		fi;\
		destDir=`cd "$$destDir"; pwd`;\
	fi;\
	if [ $$permCheck = false ]; then \
		own= grp= owngrp=;\
		dmode=755 fmode=644;\
	else \
		if [ `uname -s` = Darwin ]; then \
			fmt=-f perm='%p';\
		else \
			fmt=-c perm='%a';\
		fi;\
		eval `stat $$fmt 'own=%u grp=%g' "$$destDir"`;\
		if [ $$own -ne $$myID ] && [ $$myID -ne 0 ]; then \
			owngrp=;\
		else \
			owngrp="-o $$own -g $$grp";\
		fi;\
		eval `stat $$fmt $$perm "$$destDir" |\
		 sed 's/^/000/; s/^.*\(.\)\(.\)\(.\)\(.\)$$/p3=\1 p2=\2 p1=\3 p0=\4/'`;\
		dmode=$$p3$$p2$$p1$$p0 fmode=$$p3$$(($$p2 & 6))$$(($$p1 & 6))$$(($$p0 & 6));\
		[ $$p3 -gt 0 ] && comp=;\
	fi;\
	[ -f $(LibName) ] || { echo "Error: File '`pwd`/$(LibName)' does not exist" 1>&2; exit 1; };\
	[ -d "$$destDir"/$(LibDir) ] || install -v -d $$owngrp -m $$dmode "$$destDir"/$(LibDir) 1>&2;\
	install -v $$comp $$owngrp -m $$fmode $(LibName) "$$destDir"/$(LibDir) 1>&2;\
	[ -d "$$destDir/$(InclDir)" ] || install -v -d $$owngrp -m $$dmode "$$destDir/$(InclDir)" 1>&2;\
	for f in $(InclDir)/$(HdrFileGlob); do \
		if [ -d $$f ]; then \
			install -v -d $$owngrp -m $$dmode "$$destDir/$$f" 1>&2;\
			install -v $$comp $$owngrp -m $$fmode $$f/* "$$destDir/$$f" 1>&2;\
		else \
			install -v $$comp $$owngrp -m $$fmode $$f "$$destDir/$(InclDir)" 1>&2;\
		fi;\
	done;\
	cd $(SrcManDir) || exit $$?;\
	[ -d "$$destDir"/$(ManDir) ] || install -v -d $$owngrp -m $$dmode "$$destDir"/$(ManDir) 1>&2;\
	for n in 3 7; do \
		[ -d "$$destDir"/$(ManDir)/man$$n ] || install -v -d $$owngrp -m $$dmode "$$destDir"/$(ManDir)/man$$n 1>&2;\
		for x in *.$$n; do \
			if [ -L $$x ]; then \
				umask 133;\
				cp -v -R $$x "$$destDir"/$(ManDir)/man$$n 1>&2;\
				if [ -n "$$owngrp" ] && [ `uname -s` = Darwin ]; then \
					chown -h "$$own:$$grp" "$$destDir"/$(ManDir)/man$$n 1>&2;\
				fi;\
			else \
				umask 022;\
				install -v $$comp $$owngrp -m $$fmode $$x "$$destDir"/$(ManDir)/man$$n 1>&2;\
			fi;\
		done;\
	done;\
	echo "Done.  $(ProjName) library files installed in '$$destDir'." 1>&2

user-install:
	@echo "Beginning test software installation..." 1>&2;\
	[ -d ~/$(DestTestDir) ] || install -v -d ~/$(DestTestDir) 1>&2;\
	cd $(SrcTestDir) || exit $$?;\
	for f in $(TestFiles); do \
		d=`dirname $$f`;\
		if [ -n "$$dirname" ]; then \
			[ -d ~/$(DestTestDir)/$$d ] || install -v -d ~/$(DestTestDir)/$$d 1>&2;\
		else \
			d=.;\
		fi;\
		install -v -C -m 640 -b $$f ~/$(DestTestDir)/$$d 1>&2;\
	done;\
	cd ..;\
	install -v -C -m 640 $(SrcTestInstructions) ~/$(DestTestDir)/$(DestTestInstructions) 1>&2;\
	echo "Done.  $(ProjName) test files installed in '`cd; pwd`/$(DestTestDir)'." 1>&2

clean:
	@rm -f $(LibName) $(ObjDir)/*.o;\
	echo '$(ProjName) binaries deleted.' 1>&2
