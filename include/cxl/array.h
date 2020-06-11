// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// array.h		Definitions and declarations for creating and managing Array objects.

#ifndef array_h
#define array_h

#include "cxl/datum.h"

#define ArrayChunkSize	8		// Starting size of array element chunks.
#define ArraySizeMax	LONG_MAX	// Maximum number of elements in an array.

// Array object.
typedef ssize_t ArraySize;
typedef struct {
	ArraySize size;			// Number of elements allocated.
	ArraySize used;			// Number of elements currently in use.
	Datum **elements;		// Array of (Datum *) elements.  Elements 0..(used - 1) always point to allocated
					// Datum objects and elements used..(size - 1) are always undefined (un-allocated).
	} Array;

// External function declarations.
extern Array *aclear(Array *pArray);
extern Array *aclone(Array *pArray);
extern void acompact(Array *pArray);
extern Datum *adelete(Array *pArray, ArraySize index);
extern Datum *aeach(Array **ppArray);
extern bool aeq(const Array *pArray1, const Array *pArray2, bool ignore);
extern Datum *aget(Array *pArray, ArraySize index, bool force);
extern Array *agraft(Array *pArray1, const Array *pArray2);
extern Array *ainit(Array *pArray);
extern int ainsert(Array *pArray, ArraySize index, Datum *pVal, bool copy);
extern int ajoin(Datum *pDest, const Array *pArray, const char *delim);
extern Array *anew(ArraySize len, const Datum *pVal);
extern Datum *apop(Array *pArray);
extern int apush(Array *pArray, Datum *pVal, bool copy);
extern Datum *ashift(Array *pArray);
extern Array *aslice(Array *pArray, ArraySize index, ArraySize len, bool force, bool cut);
extern Array *asplit(short delim, const char *src, int limit);
extern int aunshift(Array *pArray, Datum *pVal, bool copy);
#endif
