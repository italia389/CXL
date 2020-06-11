// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// strip.c		Routine for removing whitespace from a string.

#include <ctype.h>
#include <string.h>

// Remove white space from given string.  If loc < 0, remove from beginning of string only; if loc > 0, remove from end of
// string only; otherwise (loc == 0), remove from both ends.  Return pointer to beginning of stripped string.  Source string
// will likely be modified by this routine.
char *strip(char *str, int loc) {

	if(*str != '\0') {

		// Strip beginning.
		if(loc <= 0)
			while(isspace(*str))
				++str;

		// Strip end.
		if(loc >= 0) {
			char *str1 = strchr(str, '\0');
			while(str1 > str && isspace(str1[-1]))
				--str1;
			if(*str1 != '\0')
				*str1 = '\0';
			}
		}

	return str;
	}
