# Root makefile for CXL library.	Ver. 1.0.0

# Definitions.
Lib = libcx.a
Lib2 = libcx2.a
Libname = cxl
InclDir = include
Install1 = /usr/local

# Targets.
.PHONY: all cxlib link uninstall install clean

all: cxlib link

cxlib:
	@if [ `uname -s` = Darwin ]; then platform=macos; else platform=linux; fi;\
	if [ ! -f $$platform/$(Lib) ]; then \
		echo 'Building CXL library...' 1>&2;\
		cd $$platform || exit $$?;\
		exec make;\
	fi

link:
	@if [ `uname -s` = Darwin ]; then platform=macos; else platform=linux; fi;\
	for x in $(Lib) $(Lib2); do \
		if [ -f $$platform/$$x ]; then \
			rm -f $$x;\
			ln -s $$platform/$$x $$x;\
			[ $$platform = macos ] && chmod -h 640 $$x;\
			echo "'$$x' link file created -> $$platform/$$x" 1>&2;\
		fi;\
	done

uninstall:
	@echo 'Uninstalling...' 1>&2;\
	if [ -n "$(INSTALL)" ] && [ -f "$(INSTALL)/lib/$(Lib)" ]; then \
		destDir="$(INSTALL)";\
	elif [ -f "$(Install1)/lib/$(Lib)" ]; then \
		destDir=$(Install1);\
	else \
		destDir=;\
	fi;\
	if [ -z "$$destDir" ]; then \
		echo "'$(Lib)' library not found." 1>&2;\
	else \
		for f in $$destDir/lib/$(Lib) $$destDir/include/{$(Libname),stdos.h}; do \
			if [ -e "$$f" ]; then \
				rm -rf "$$f" && echo "Deleted '$$f'" 1>&2;\
			fi;\
		done;\
	fi;\
	echo 'Done.' 1>&2

install: uninstall
	@echo 'Beginning installation...' 1>&2;\
	umask 022;\
	myID=`id -u`;\
	comp='-C' permCheck=true destDir=$(INSTALL);\
	if [ $$myID -ne 0 ]; then \
		if [ -z "$$destDir" ]; then \
			if uname -v | fgrep -qi debian; then \
				echo 'Error: You must be root to perform a site-wide install' 1>&2;\
				exit 1;\
			fi;\
		else \
			permCheck=false;\
		fi;\
	fi;\
	[ -z "$$destDir" ] && destDir=$(Install1);\
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
	[ -f $(Lib) ] || { echo "Error: File '`pwd`/$(Lib)' does not exist" 1>&2; exit 1; };\
	[ -d "$$destDir"/lib ] || install -v -d $$owngrp -m $$dmode "$$destDir"/lib 1>&2;\
	install -v $$owngrp -m $$fmode $(Lib) "$$destDir"/lib 1>&2;\
	[ -d "$$destDir/$(InclDir)" ] || install -v -d $$owngrp -m $$dmode "$$destDir/$(InclDir)" 1>&2;\
	install -v $$owngrp -m $$fmode $(InclDir)/stdos.h "$$destDir/$(InclDir)" 1>&2;\
	[ -d "$$destDir/$(InclDir)/$(Libname)" ] || install -v -d $$owngrp -m $$dmode "$$destDir/$(InclDir)/$(Libname)" 1>&2;\
	install -v $$owngrp -m $$fmode $(InclDir)/$(Libname)/* "$$destDir/$(InclDir)/$(Libname)" 1>&2;\
	echo "Done.  CXL files installed in '$$destDir'." 1>&2

clean:
	@if [ `uname -s` = Darwin ]; then platform=macos; else platform=linux; fi;\
	cd $$platform || exit $$?;\
	make $@; cd ..;\
	echo "'$$platform' binaries deleted." 1>&2;\
	if [ -L $(Lib) ]; then \
		rm -f $(Lib) && echo "'$(Lib)' link file deleted." 1>&2;\
	fi
