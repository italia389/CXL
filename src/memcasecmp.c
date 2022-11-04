// (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// memcasecmp.c			Compare two byte strings, ignoring case.

#include <stddef.h>

#define lowCase(c)	((c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c)

// Compare two byte strings, folding upper-case letters to lower case.  Return an integer less than zero (str1 < str2),
// zero (str1 == str2), or greater than zero (str1 > str2).
int memcasecmp(const void *str1, const void *str2, size_t len) {
	short c1, c2;
	char *memStr1 = (char *) str1;
	char *memStr2 = (char *) str2;

	for(c1 = c2 = 0; len > 0; --len) {
		c1 = *memStr1++;
		c2 = *memStr2++;
		if(lowCase(c1) != lowCase(c2))
			break;
		}
	return c1 - c2;
	}
