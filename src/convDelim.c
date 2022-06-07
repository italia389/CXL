// (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// convDelim.c		Convert a delimiter specification to an integer.

#include "stdos.h"
#include "cxl/excep.h"
#include <stdlib.h>
#include <errno.h>

// Convert given delimiter specification (string) to an ASCII character and return it.  If string is invalid, set an exception
// message and return -1.  Specification argument must be a single character or a numeric literal.  If the latter, the value
// must be in range 0x00 - 0xFF (the value of an 8-bit ASCII character) and conform to the string requirements of strtol() with
// base zero.  In either case, leading or trailing white space is considered an error.
int convDelim(const char *spec) {

	// Non-empty string?
	if(spec[0] != '\0') {

		// Yes, numeric literal?
		if(spec[0] >= '0' && spec[0] <= '9') {
			char *str;
			ulong delim;
		
			errno = 0;
			delim = strtoul(spec, &str, 0);
			if(*str == '\0' && errno == 0 && delim <= 0xFF)
				return (int) delim;
			}

		// No, single character?
		else if(spec[1] == '\0')
			return (int) spec[0];
		}

	// It's a bust.  Return exception.
	return emsgf(-1, "Invalid delimter '%s' (must be a character or ASCII value)", spec);
	}
