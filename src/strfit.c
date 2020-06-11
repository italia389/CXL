// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// strfit.c		Routine for fitting a string into a buffer.

#include "stdos.h"
#include "cxl/string.h"
#include <string.h>

// Copy and shrink string by inserting an ellipsis in the middle (if necessary), given destination pointer, maximum destination
// length (excluding the trailing null), source pointer, and source length.  Destination buffer is assumed to be at least
// maxLen + 1 bytes in size.  If source length is zero, source string is assumed to be null terminated.  Return dest.
char *strfit(char *dest, size_t maxLen, const char *src, size_t srcLen) {
	size_t srcLen1, len, cutSize;
	const char *ellipsis;
	char *str;

	// Check for minimum shrinking parameters.
	srcLen1 = (srcLen == 0) ? strlen(src) : srcLen;
	if(maxLen < 5 || srcLen1 <= maxLen)

		// Dest or source too short, so just copy the maximum.
		(void) stplcpy(dest, src, (srcLen1 < maxLen ? srcLen1 : maxLen) + 1);
	else {
		// Determine maximum number of characters can copy from source.
		if(maxLen < 30) {
			ellipsis = "..";			// Use shorter ellipsis for small dest.
			len = 2;
			}
		else {
			ellipsis = "...";
			len = 3;
			}
		cutSize = srcLen1 - maxLen + len;		// Number of characters to leave out of middle of source.
		len = (srcLen1 - cutSize) / 2;			// Length of initial segment (half of what will be copied).
								// Include white space at end first segment if present.
		str = (char *) src + len;
		if((str[0] == ' ' || str[0] == '\t') &&
		 str[-1] != ' ' && str[-1] != '\t')
			++len;
		str = stplcpy(dest, src, len + 1);		// Copy initial segment.
		str = stpcpy(str, ellipsis);			// Add ellipsis.
								// Copy last segment.
		stplcpy(str, src + cutSize + len, srcLen1 - cutSize - len + 1);
		}

	return dest;
	}
