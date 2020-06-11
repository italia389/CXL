// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// stplcpy.c		Routine for copying a string with overflow prevention.

#include <stddef.h>

// Copy a string ... with length restrictions.  Null terminate result if 'size' is greater than zero.  'size' is the actual size
// of the destination buffer.  Copying stops when 'size' - 1 bytes have been copied or null byte is found in 'src'.  Return
// pointer to terminating null in 'dest' or 'dest' if 'size' is zero.
char *stplcpy(char *dest, const char *src, size_t len) {

	if(len > 0) {
		while(--len > 0 && *src != '\0')
			*dest++ = *src++;
		*dest = '\0';
		}
	return dest;
	}
