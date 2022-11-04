// (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// join.c		Routine for joining array elements of a StrArray object together into a single string.

#include "stdos.h"
#include "cxl/string.h"
#include "cxl/datum.h"
#include <string.h>
#include <stdlib.h>

// Concatenate elements of string array src together using delimiter delim (if not NULL) as "glue" and save result in given
// Datum object.  Return status code.
int join(Datum *pDest, StrArray *pSrc, const char *delim) {

	// Empty array?
	if(pSrc->itemCount == 0)
		dsetnull(pDest);
	else {
		char **pStr;
		DFab fab;

		// Open fabrication object with caller's Datum object.
		if(dopenwith(&fab, pDest, FabClear) != 0)
			return -1;

		// Join array elements.
		pStr = pSrc->items;
		do {
			if(delim != NULL && pStr != pSrc->items && bputs(delim, &fab) != 0)
				return -1;
			if(bputs(*pStr++, &fab) != 0)
				return -1;
			} while(*pStr != NULL);

		// Close fabrication object.
		if(dclose(&fab, FabStr) != 0)
			return -1;
		}

	return 0;
	}
