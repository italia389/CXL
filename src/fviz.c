// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// fputviz.c		Routines for writing characters and strings to a file in visible form.

#include "stdos.h"
#include "cxl/excep.h"
#include "cxl/string.h"
#include <stdio.h>
#include <string.h>

// Write a character to given file in visible form.  Pass 'flags' to vizc().  Return status code.
int fputvizc(short c, ushort flags, FILE *stream) {
	char *str;

	return (str = vizc(c, flags)) == NULL ? -1 : fputs(str, stream) == EOF ? emsgsys(-1) : 0;
	}

// Write bytes to given file, exposing all invisible characters.  Pass 'flags' to vizc().  If size is zero, assume 'memPtr' is a
// null-terminated string; otherwise, write exactly 'size' bytes.  Return status code.
int fputvizmem(const void *memPtr, size_t size, ushort flags, FILE *stream) {
	char *memPtr1 = (char *) memPtr;
	size_t n = (size == 0) ? strlen(memPtr1) : size;

	for(; n > 0; --n)
		if(fputvizc(*memPtr1++, flags, stream) != 0)
			return -1;
	return 0;
	}
