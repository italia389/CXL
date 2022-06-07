// (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// split.c		Routine for splitting a string into an array (which is contained in a StrArray object).

#include "stdos.h"
#include "cxl/excep.h"
#include "cxl/string.h"
#include <string.h>
#include <stdlib.h>

// Split a string into an array of substrings.  Substrings are separated by a single character, white space, or no delimiter, as
// indicated by integer value "delim":
//	VALUE		MEANING
//	0 or ' '	Delimiter is white space.  One or more of the characters (' ', '\t', '\n', '\r', '\f', '\v') are treated
//			as a single delimiter.  Additionally, if the delimiter is ' ', leading white space is ignored.
//	1 - 0xff	ASCII value of a character to use as the delimiter (except ' ').
//	> 0xff		No delimiter is defined.  A single-element array is returned containing the original string, unmodified.
//
// The "limit" argument controls the splitting process, as follows:
//	VALUE		MEANING
//	< 0		Each delimiter found is significant and delineates two substrings, either or both of which may be null.
//	0		Trailing null substrings are suppressed.
//	> 0		Maximum number of array elements to return.  The last element of the array will contain embedded
//			delimiter(s) if the number of delimiters in the string is equal to or greater than the "limit" value.
//
// Routine saves results in a new StrArray object containing an additional pointer element set to NULL (to indicate end-of-
// array), and returns a pointer to the object, or NULL if error.  Delimiters in the source string are replaced by nulls as the
// string is parsed; hence, the source string will be modified by this routine if any delimiters are found.
StrArray *split(short delim, char *src, int limit) {
	uint itemCount;
	char *str0, *str, **pStr;
	StrArray *pStrArray;
	char delims[7];

	// Count tokens.
	itemCount = 0;
	if(*src != '\0') {

		// Set actual delimiter string.
		if(delim > 0xFF)
			delims[0] = '\0';
		else if(delim > 0 && delim != ' ') {
			delims[0] = delim;
			delims[1] = '\0';
			}
		else
			strcpy(delims, " \t\n\r\f\v");

		// Skip leading white space if requested.
		str = src;
		if(delim == ' ')
			for(;;) {
				if(strchr(delims, *str) == NULL)
					break;
				if(*++str == '\0')
					goto Done;
				}

		// Scan string.
		for(;;) {
			++itemCount;

			// Find next delimiter; first character may be null.
			if((limit > 0 && itemCount == (unsigned) limit) || (str = strpbrk(str, delims)) == NULL)
				break;

			// Found delimiter.  Check for run if delimter is white space or limit is zero.
			if(delims[1] != '\0' || limit == 0)
				for(str0 = str; ; ++str) {
					if(str[1] == '\0') {
						if(limit != 0)
							break;

						// Truncate trailing delimiters.
						*(str = str0) = '\0';
						goto Done;
						}
					if(strchr(delims, str[1]) == NULL) {

						// Not at end of string.  Ignore run if delimiter is not white space.
						if(delims[1] == '\0')
							str = str0;
						break;
						}
					}
			// Onward.
			++str;
			}
		}
Done:
	// Allocate string array, which will contain at least one element.
	if((pStrArray = (StrArray *) malloc(sizeof(StrArray) + itemCount * sizeof(char *))) == NULL) {
		cxlExcep.flags |= ExcepMem;
		(void) emsgsys(-1);
		return NULL;
		}
	pStrArray->itemCount = itemCount;

	// Store substring pointers.
	pStr = pStrArray->items;
	if(itemCount > 0) {
		str = src;
		if(delim == ' ')
			while(strchr(delims, *str) != NULL)
				++str;
		for(;;) {
			*pStr++ = str;
			if(--itemCount == 0)
				break;
			(void) strsep(&str, delims);		// New value of str will never be NULL.
			if(delims[1] != '\0')
				while(*str != '\0' && strchr(delims, *str) != NULL)
					++str;
			}
		}
	*pStr = NULL;

	return pStrArray;
	}
