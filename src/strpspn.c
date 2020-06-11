// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// strpspn.c		Routine for spanning a string.

#include "stdos.h"
#include <stddef.h>
#include <string.h>

// Find first character in str1 that does not occur in str2 and return pointer to it.  If all characters in str1 occur in str2,
// a pointer to the terminating null in str1 is returned.
char *strpspn(const char *str1, const char *str2) {
	short c;

	if(*str2 == '\0')
		return (char *) str1;
	while((c = *str1) != '\0') {
		if(strchr(str2, c) == NULL)
			break;
		++str1;
		}
	return (char *) str1;
	}
