// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// fviz.c		Routines for writing characters and strings to a file in visible form.

#include "stdos.h"
#include "cxl/excep.h"
#include "cxl/string.h"
#include <stdio.h>
#include <string.h>

// Write a character to given file in visible form.  Pass 'flags' to vizc().  Return status code.
int fvizc(short c, ushort flags, FILE *pFile) {
	char *str;

	return (str = vizc(c, flags)) == NULL ? -1 : fputs(str, pFile) == EOF ? emsgsys(-1) : 0;
	}

// Write bytes to given file, exposing all invisible characters.  Pass 'flags' to vizc().  If size is zero, assume 'mem' is a
// null-terminated string; otherwise, write exactly 'size' bytes.  Return status code.
int fvizmem(const void *mem, size_t size, ushort flags, FILE *pFile) {
	char *mem1 = (char *) mem;
	size_t n = (size == 0) ? strlen(mem1) : size;

	for(; n > 0; --n)
		if(fvizc(*mem1++, flags, pFile) != 0)
			return -1;
	return 0;
	}
