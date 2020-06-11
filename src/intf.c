// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// intf.c		Routine for converting an integer to a string with embedded commas.

#include "stdos.h"

// Convert given long integer to a string (allocated in static storage) with a leading sign if negative and commas every three
// digits.  Return result.
char *intf(long i) {
	bool neg = false;
	int n = 0;
	char *str;
	static char result[sizeof(long) * 4];

	if(i < 0) {
		neg = true;
		i = -i;
		}
	str = result + sizeof(result);
	*--str = '\0';
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
