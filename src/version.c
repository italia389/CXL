// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// version.c		Routine for returning the CXL library version.

#include <stdio.h>
#include "cxl/lib.h"

// Return human-readable library version string.
char *cxlvers(void) {
	static char libVer[32];

	if(libVer[0] == '\0')
		sprintf(libVer, "CXL %s (GPLv3)", CXL_Version);
	return libVer;
	}
