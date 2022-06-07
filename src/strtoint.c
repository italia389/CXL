// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// strtoint.c		Routines for converting a numeric string literal to an integer with exception handling.

#include "stdos.h"
#include "cxl/excep.h"
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

// Convert a numeric literal to a signed or unsigned long integer via strtol() or strtoul() and save result in *pLong or
// *pULong.  If an error occurs, set exception message and return -1; otherwise, return 0.  If the input string contains any
// non-whitespace character following the numeric literal, it is considered an error.
static int econvert(const char *str, int base, long *pLong, ulong *pULong, const char *signType) {
	char *str1;
	long lResult;
	ulong uResult;

	errno = 0;
	if(pLong != NULL)
		lResult = strtol(str, &str1, base);
	else
		uResult = strtoul(str, &str1, base);
	if(errno == 0 && str1 > str) {
		while(isspace(*str1))
			++str1;
		if(*str1 == '\0') {
			if(pLong != NULL)
				*pLong = lResult;
			else
				*pULong = uResult;
			return 0;
			}
		}

	// It's a bust.  Return exception.
	return emsgf(-1, "Invalid base %d %ssigned number '%s'", base, signType, str);
	}

// Convert a numeric literal to a signed long integer via econvert().
int estrtol(const char *str, int base, long *pResult) {

	return econvert(str, base, pResult, NULL, "");
	}

// Convert a numeric literal to an unsigned long integer via econvert().
int estrtoul(const char *str, int base, ulong *pResult) {

	return econvert(str, base, NULL, pResult, "un");
	}
