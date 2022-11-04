// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// array.h		Definitions and declarations for creating and managing Array objects.

#ifndef array_h
#define array_h

#include <stdint.h>
#include "cxl/datum.h"

#define ArrayChunkSize	8		// Starting size of array element chunks.
#define ArraySizeMax	LONG_MAX	// Maximum number of elements in an array.

// Flags for controlling function options (aflags).
#define AOpIgnore	0x0001		// Ignore case in string comparisons.
#define AOpCopy		0x0002		// Copy datum into array, otherwise move.
#define AOpCut		0x0004		// Delete array elements after operation completed.
#define AOpGrow		0x0008		// Expand array as needed to complete operation.
#define AOpInPlace	0x0010		// Perform operation on first array argument; otherwise, make new array.
#define AOpLast		0x0020		// Find last occurrence, otherwise first.
#define AOpNonMatching	0x0040		// Return non-matching elements, otherwise matching.

// For internal use.
#define AOpRemaining	0x1000		// Excessive length parameter indicates "all remaining elements".
#define AOpFill		0x2000		// Fill extended array slots with nil values in aneed().
#define AOpCloneAll	0x4000		// Force copying of all arrays, including references (when cloning).

// Array object.
typedef ssize_t ArraySize;
typedef struct Array {
	struct Array *next;		// Pointer to next array on (garbage) list -- for external use.
	bool tagged;			// Array is tagged (for garbage collection) -- for external use.
	uint32_t id;			// Random ID number used to detect an array that includes itself.
	ArraySize size;			// Number of elements allocated.
	ArraySize used;			// Number of elements currently in use.
	Datum **elements;		// Array of (Datum *) elements.  Elements 0..(used - 1) always point to allocated
					// Datum objects and elements used..(size - 1) are always undefined (un-allocated).
	} Array;

#define aempty(array)	((array)->used == 0)

// External function declarations.
extern Array *acat(Array *pArray1, const Array *pArray2, ushort aflags);
extern void aclear(Array *pArray);
extern Array *aclone(const Array *pArray);
extern Array *acompact(Array *pArray, ushort aflags);
extern Datum *adelete(Array *pArray, ArraySize index);
extern ArraySize adeleteif(Array *pArray, const Datum *pVal, ushort aflags);
extern Datum *aeach(Array **ppArray);
extern bool aeq(const Array *pArray1, const Array *pArray2, ushort aflags);
extern int afill(Array *pArray, const Datum *pVal, ArraySize index, ArraySize len, ushort aflags);
extern void afree(Array *pArray);
extern Datum *aget(Array *pArray, ArraySize index, ushort aflags);
extern bool ainclude(const Array *pArray, const Datum *pVal, ushort aflags);
extern ArraySize aindex(const Array *pArray, const Datum *pVal, ushort aflags);
extern void ainit(Array *pArray);
extern int ainsert(Array *pArray, Datum *pVal, ArraySize index, ushort aflags);
extern int ajoin(Datum *pDatum, const Array *pArray, const char *delim, ushort cflags);
extern Array *amatch(Array *pArray1, const Array *pArray2, ushort aflags);
extern Array *anew(ArraySize len, const Datum *pVal);
extern Datum *apop(Array *pArray);
extern int apush(Array *pArray, Datum *pVal, ushort aflags);
extern int aput(const Array *pArray, DFab *pFab, const char *delim, ushort cflags);
extern Datum *ashift(Array *pArray);
extern Array *aslice(Array *pArray, ArraySize index, ArraySize len, ushort aflags);
extern Array *asplit(short delim, const char *src, int limit);
extern int atos(Datum *pDatum, const Array *pArray, const char *delim, ushort cflags);
extern Array *auniq(Array *pArray1, const Array *pArray2, ushort aflags);
extern int aunshift(Array *pArray, Datum *pVal, ushort aflags);
#endif
