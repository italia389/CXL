// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// strrev.c		Reverse a string in place.

#include <string.h>
#include <wchar.h>
#include "cxl/excep.h"

// Reverse a string in place and return it.
char *strrev(char *str) {
	char *strEnd = strchr(str, '\0');
	if(strEnd - str > 1) {
		short c;
		char *str1 = str;
		do {
			c = *str1;
			*str1++ = *--strEnd;
			*strEnd = c;
			} while(strEnd > str1 + 1);
		}
	return str;
	}

// Reverse a byte string in place and return it.
void *memrev(void *str, size_t len) {

	if(len > 1) {
		short c;
		char *str1 = (char *) str;
		char *strEnd = str1 + len;
		do {
			c = *str1;
			*str1++ = *--strEnd;
			*strEnd = c;
			} while(strEnd > str1 + 1);
		}
	return str;
	}

// Reverse a multibyte string in place and return status code.
int mbrev(char *str, size_t len) {

	if(len > 1) {
		mbstate_t mbstate;
		size_t n;
		char *strEnd, *str1;

		// First, find and reverse any multibyte sequences within the string.
		strEnd = (str1 = str) + len;
		memset(&mbstate, '\0', sizeof(mbstate));
		do {
			if((*str1 & 0x80) == 0)
				goto Incr;
			if((n = mbrlen(str1, strEnd - str1, &mbstate)) == (size_t) -1 || n == (size_t) -2)
				return emsgf(-1, "Invalid multibyte character in string (at offset %lu)", str1 - str);
			else if(n == 0)
Incr:
				++str1;
			else {
				memrev(str1, n);
				str1 += n;
				}
			} while(str1 < strEnd);

		// Now reverse the whole string.
		memrev(str, len);
		}

	return 0;
	}
