// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// intf.c		Routines for converting a signed or unsigned integer to a string with commas in the thousands' places.

#include "stdos.h"
#include <stdbool.h>

static char result[sizeof(ulong) * 4];

// Convert given long integer to a string (allocated in static storage) with a leading sign if "neg" is true, and commas every
// three digits.  Return result.
static char *iconv(ulong i, bool neg) {
	int n = 0;
	char *str;

	str = result + sizeof(result) - 1;
	*str = '\0';
	do {
		if(n++ == 3) {
			*--str = ',';
			n = 1;
			}
		*--str = i % 10 + '0';
		} while((i /= 10) > 0);
	if(neg)
		*--str = '-';

	return str;
	}

char *intf(long i) {

	return iconv(i < 0 ? -i : i, i < 0);
	}

char *uintf(ulong i) {

	return iconv(i, false);
	}
