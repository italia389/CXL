// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// strcbrk.c		Routine for locating multiple characters in a string.

#include "stdos.h"
#include <stddef.h>
#include <string.h>

// Find first character in str that does not occur in charset and return pointer to it.  If all characters in str occur in
// charset, a pointer to the terminating null of str is returned.
char *strcbrk(const char *str, const char *charset) {
	short c;

	if(*charset == '\0')
		return (char *) str;
	while((c = *str) != '\0') {
		if(strchr(charset, c) == NULL)
			break;
		++str;
		}
	return (char *) str;
	}
