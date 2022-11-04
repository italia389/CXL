// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// stplvizcpy.c		Routine for copying a string in visible form with overflow prevention.

#include "stdos.h"
#include "cxl/string.h"
#include <stddef.h>
#include <string.h>

// Copy a string in visible form ... with length restrictions.  Each byte in 'src' is passed to vizc() with 'flags' argument and
// the result is stored in 'dest'.  The string in 'dest' is null-terminated if 'destLen' (which is the actual size of the
// destination buffer) is greater than zero.  Copying stops when 'destLen' - 1 bytes have been saved in 'dest' or 'srcLen' bytes
// have been converted from 'src'.  Return pointer to terminating null in 'dest' or 'dest' if 'destLen' is zero.
char *stplvizcpy(char *dest, size_t destLen, const void *src, size_t srcLen, ushort flags) {

	if(destLen > 0) {
		char *vizChar;
		size_t vizLen;
		const char *src1, *srcEnd;

		srcEnd = (src1 = src) + srcLen;
		while(src1 < srcEnd) {
			vizLen = strlen(vizChar = vizc(*src1++, flags));
			if(vizLen >= destLen)
				break;
			dest = memstpcpy(dest, vizChar, vizLen);
			destLen -= vizLen;
			}
		*dest = '\0';
		}
	return dest;
	}
