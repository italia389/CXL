// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// array.c		Array-handling routines.
//
// Notes:
//  1. Any array index argument in a function call may be negative, in which case it addresses the nth element from the end,
//     where -1 is the last element, -2 is second from last, etc.
//  2. Any length argument in a function call may be negative, which indicates "all remaining elements".  It may also be a
//     positive number that is larger than the number of elements remaining in the array if (1), it references a subarray to be
//     extracted (e.g., in aslice()), in which case it also indicates "all remaining elements", or (2), it references a subarray
//     to be operated on (e.g., in afill()) and the AOpGrow flag is also specified, in which case it causes the array to be
//     auto-extended; otherwise, it is an error.

#include "stdos.h"
#include "cxl/excep.h"
#include "cxl/datum.h"
#include "cxl/array.h"
#include "cxl/lib.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>

// Global variables (used to detect if an array contains itself).
static uint32_t arrayID = 0;			// Random ID number.
static uint arrayNestLevel = 0;			// Array nesting level.

// Initialize an Array object as an empty array.
void ainit(Array *pArray) {

	*pArray = (Array) {NULL};	// Set contents to zero.
	}

// Clear an array: free element storage (if any) and set used size to zero.
void aclear(Array *pArray) {
	Datum **ppArrayEl = pArray->elements;
	if(ppArrayEl != NULL) {
		Datum **ppArrayElEnd = ppArrayEl + pArray->used;
		while(ppArrayEl < ppArrayElEnd)
			dfree(*ppArrayEl++);
		free((void *) pArray->elements);
		}
	pArray->used = pArray->size = 0;
	pArray->elements = NULL;
	}

// Generate random array ID for detection of endless array recursion.
static void genArrayID(void) {

	arrayID = rand32Uniform(UINT32_MAX - 1) + 1;	// Will never be zero.
	}

// Free an array.
void afree(Array *pArray) {

	// Generate new array ID if at top-level.
	if(arrayNestLevel == 0)
		genArrayID();

	// Clear and nuke this array unless it has been seen before (and is being nuked farther up the recursion stack).
	if(pArray->id != arrayID) {
		++arrayNestLevel;
		pArray->id = arrayID;
		aclear(pArray);
		free((void *) pArray);
		--arrayNestLevel;
		}
	}

// Plug nil values into array slot(s), given index and length.  It is assumed that all slots have been allocated and are
// available for reassignment.  Return status code.
static int plugNil(Array *pArray, ArraySize index, ArraySize len) {

	if(len > 0) {
		Datum **ppArrayEl = pArray->elements + index;
		Datum **ppArrayElEnd = ppArrayEl + len;
		do {
			if(dnew(ppArrayEl++) != 0)
				return -1;
			} while(ppArrayEl < ppArrayElEnd);
		}
	return 0;
	}

// Check if array needs to grow so that it contains given array index (which mandates that array contain at least index + 1
// elements) or to accomodate given increase to "used" size.  Get more space if needed: use ArrayChunkSize if first allocation,
// double old size for second and third, then increase old size by 3/4 thereafter (to prevent the array from growing too fast
// for large numbers).  Return error if size needed exceeds maximum.  Any slots allocated beyond the current used size are left
// undefined if AOpFill flag not set; otherwise, new slots up to and including the index are set to nil and the used size is
// updated.  It is assumed that only one of "growSize" or "index" will be valid, so first found with valid value is used.
static int need(Array *pArray, ArraySize growSize, ArraySize index, ushort aflags) {
	ArraySize minSize;

	// Increasing "used" size?
	if(growSize > 0) {
		if(growSize <= pArray->size - pArray->used)	// Is unused portion of array big enough?
			goto CheckFill;				// Yes, check if fill requested.
		if(growSize > ArraySizeMax - pArray->used)	// No, too small.  Does request exceed maximum?
			goto TooMuch;				// Yes, error.
		minSize = pArray->used + growSize;		// No, compute new minimum size.
		}
	else {
		if(index < pArray->used)			// Is index within "used" bounds?
			return 0;				// Yes, nothing to do.
		growSize = index + 1 - pArray->used;		// No, compute grow size.
		if(index < pArray->size)			// Is index within "size" bounds?
			goto CheckFill;				// Yes, check if fill requested.
		if(index == ArraySizeMax || pArray->size == ArraySizeMax) // Error: maximum size exceeded.
TooMuch:
			return emsgf(-1, "Cannot grow array beyond maximum size (%ld)", ArraySizeMax);
		minSize = index + 1;
		}

	// Expansion needed.  Increase old size until it's big enough, without causing an integer overflow.
	ArraySize newSize = pArray->size;
	do {
		newSize = (newSize == 0) ? ArrayChunkSize : (newSize < ArrayChunkSize * 4) ? newSize * 2 :
		 (ArraySizeMax - newSize < newSize) ? ArraySizeMax : newSize + (newSize >> 2) * 3;
		} while(newSize < minSize);

	// Get more space.
	if((pArray->elements = (Datum **) realloc((void *) pArray->elements, newSize * sizeof(Datum *))) == NULL) {
		cxlExcep.flags |= ExcepMem;
		return emsgf(-1, "%s, allocating %ld-element array", strerror(errno), newSize);
		}
	pArray->size = newSize;
CheckFill:
	if(aflags & AOpFill) {
		// Set new elements to nil and increase "used" size.
		if(plugNil(pArray, pArray->used, growSize) != 0)
			return -1;
		pArray->used += growSize;
		}

	return 0;
	}

// Validate and normalize array slice parameters (convert to non-negative values).  Recognize negative index as an "nth element
// from end" selector (as long as it does not exceed array length) and negative length as "all elements remaining".  Also allow
// index and/or slice to be beyond end of array if AOpGrow flag set, and length to be "all elements remaining" if slice extends
// past end of array, AOpGrow flag not set, but AOpRemaining flag is set.  Return status code.
static int normalize(Array *pArray, ArraySize *pIndex, ArraySize *pLen, ushort aflags) {
	ArraySize index = *pIndex;
	ArraySize len = *pLen;

	// Validate and normalize index.
	if(index < 0) {
		if(-index > pArray->used)
			goto IndexErr;
		index += pArray->used;
		}
	else if(!(aflags & AOpGrow) && index >= pArray->used)
IndexErr:
		return emsgf(-1, "Array index %ld out of range (array size %ld)", index, pArray->used);

	// Validate and normalize length.
	if(len < 0)
		len = pArray->used - index;

	// Validate slice span.
	if(!(aflags & AOpGrow) && index + len > pArray->used) {
		if(aflags & AOpRemaining)
			len = pArray->used - index;
		else
			return emsgf(-1, "Array slice parameters [%ld, %ld] out of range (array size %ld)",
			 *pIndex, *pLen, pArray->used);
		}

	// All is well.  Return normalized slice values.
	*pIndex = index;
	*pLen = len;
	return 0;
	}

// Open slot in an array at given index by shifting higher elements to the right one position (so that a value can be inserted
// there by the caller).  If AOpCopy flag is set, set slot to a nil value.  Update "used" size and return status code.  Index is
// assumed to be less than or equal to used size.
static int spread(Array *pArray, ArraySize index, ushort aflags) {

	if(need(pArray, 1, -1, 0) != 0)					// Ensure a slot exists at end of used portion.
		return -1;
	if(index < pArray->used) {					// If not inserting at end of array...
		Datum **ppArrayEl0, **ppArrayEl1;

		// 012345678	012345678	index = 1
		// abcdef...	a.bcdef..
		//  0    1
		ppArrayEl0 = pArray->elements + index;			// set slot pointers...
		ppArrayEl1 = pArray->elements + pArray->used;
		do {							// and shift elements to the right.
			ppArrayEl1[0] = ppArrayEl1[-1];
			} while(--ppArrayEl1 > ppArrayEl0);
		}
	++pArray->used;							// Bump "used" size...
	return (aflags & AOpCopy) ? plugNil(pArray, index, 1) : 0;	// and set slot to nil if requested.
	}

// Remove one or more contiguous elements from an array at given location.  It is assumed that the Datum object(s) in the slice
// have already been saved or freed and that "index" and "len" are in range.
static void shrink(Array *pArray, ArraySize index, ArraySize len) {

	if(len > 0) {
		if(index + len < pArray->used) {					// Slice at end of array?
			Datum **ppArrayEl0, **ppArrayEl1, **ppArrayEl2;			// No, close the gap.

			// 01234567	01234567	index = 1, len = 3
			// abcdef..	aef.....
			//  0  1   2
			ppArrayEl1 = (ppArrayEl0 = pArray->elements + index) + len;	// Set slot pointers...
			ppArrayEl2 = pArray->elements + pArray->size;
			do {								// and shift elements to the left.
				*ppArrayEl0++ = *ppArrayEl1++;
				} while(ppArrayEl1 < ppArrayEl2);
			}
		pArray->used -= len;							// Update "used" length.
		}
	}

// Scan array from beginning up to, but not including, given index for first (or last if AOpLast flag set) element matching
// given datum.  Ignore case in string comparisons if AOpIgnore flag set.  Index is assumed to be non-negative and less than or
// equal to array length.  Return index if matching element found, otherwise -1.
static ArraySize scan(const Array *pArray, ArraySize index, const Datum *pVal, ushort aflags) {
	Datum **ppArrayEl, **ppArrayElEnd;

	ppArrayElEnd = (ppArrayEl = pArray->elements) + index;
	index = -1;
	while(ppArrayEl < ppArrayElEnd) {
		if(deq(*ppArrayEl, pVal, aflags & AOpIgnore)) {
			index = ppArrayEl - pArray->elements;
			if(!(aflags & AOpLast))
				break;
			}
		++ppArrayEl;
		}
	return index;
	}

// Delete all elements in array that match given datum and return number of elements deleted.  Ignore case in string comparisons
// if AOpIgnore flag set.
static ArraySize deleteif(Array *pArray, const Datum *pVal, ushort aflags) {
	ArraySize count = 0;

	if(pArray->used > 0) {
		Datum **ppArrayEl0, **ppArrayEl1, **ppArrayElEnd;

		ppArrayElEnd = (ppArrayEl0 = ppArrayEl1 = pArray->elements) + pArray->used;
		do {
			if(deq(*ppArrayEl1, pVal, aflags & AOpIgnore)) {	// Match?
				dfree(*ppArrayEl1);				// Yes, delete element...
				++count;					// and count it.
				}
			else if(ppArrayEl1 == ppArrayEl0)
				++ppArrayEl0;					// No, skip over it...
			else
				*ppArrayEl0++ = *ppArrayEl1;			// or shift it left.
			} while(++ppArrayEl1 < ppArrayElEnd);

		pArray->used = ppArrayEl0 - pArray->elements;			// Update "used" length.
		}
	return count;
	}

// Create an array (in heap space) and return pointer to it, or NULL if error.  If len > 0, pre-allocate that number of elements
// and, if AOpFill flag is set, set them to nil (or *pVal if pVal not NULL); otherwise, leave undefined.
static Array *create(ArraySize len, const Datum *pVal, ushort aflags) {
	Array *pArray;

	if(len < 0) {
		emsgf(-1, "Invalid array length (%ld)", len);
		return NULL;
		}

	if((pArray = (Array *) malloc(sizeof(Array))) == NULL) {	// Get space for array object...
		cxlExcep.flags |= ExcepMem;
		emsgsys(-1);
		return NULL;
		}
	ainit(pArray);							// initialize empty array...
	if(len > 0) {							// allocate elements if requested...
		if(need(pArray, 0, len - 1, aflags) != 0)
			return NULL;
		if(pVal != NULL) {
			Datum **ppArrayEl = pArray->elements;
			do {
				if(dcpy(*ppArrayEl++, pVal) != 0)
					return NULL;
				} while(--len > 0);
			}
		}
	return pArray;							// and return it.
	}

// Create an array by calling create() with AOpFill flag.
Array *anew(ArraySize len, const Datum *pVal) {

	return create(len, pVal, AOpFill);
	}

// Scan array for first (or last if AOpLast flag set) element matching given datum.  If AOpIgnore flag is set, case in string
// comparisons is ignored.  Return index of matching element if found, otherwise -1.
ArraySize aindex(const Array *pArray, const Datum *pVal, ushort aflags) {

	return scan(pArray, pArray->used, pVal, aflags & (AOpIgnore | AOpLast));
	}

// Return true if given datum occurs in array, otherwise false.  If AOpIgnore flag is set, case in string comparisons is
// ignored.
bool ainclude(const Array *pArray, const Datum *pVal, ushort aflags) {

	return scan(pArray, pArray->used, pVal, aflags & AOpIgnore) >= 0;
	}

// Delete all elements in array equal to datum and return deletion count.  If AOpIgnore flag is set, case in string comparisons
// is ignored.
ArraySize adeleteif(Array *pArray, const Datum *pVal, ushort aflags) {

	return deleteif(pArray, pVal, aflags & AOpIgnore);
	}

// Insert a Datum object (or a copy if AOpCopy flag is set) into an array at given index position.  Return status code.
static int put(Array *pArray, ArraySize index, Datum *pDatum, ushort aflags) {

	if(spread(pArray, index, aflags) != 0)
		return -1;
	if(!(aflags & AOpCopy))
		pArray->elements[index] = pDatum;
	else if(dcpy(pArray->elements[index], pDatum) != 0)
		return -1;
	return 0;
	}

// Return copy of *pArray excluding nil elements, or if AOpInPlace flag set, remove nil elements from *pArray, shift elements
// left to fill the hole(s), and return pArray (or NULL if error).
Array *acompact(Array *pArray, ushort aflags) {
	Array *pArrayTarg;

	// Compact in place if requested.
	if(aflags & AOpInPlace) {
		Datum datum;

		pArrayTarg = pArray;
		dinit(&datum);
		(void) deleteif(pArrayTarg, &datum, 0);
		}
	else {
		// Copy non-nil elements to new array.
		Datum **ppArrayEl, **ppArrayElEnd;
		ppArrayElEnd = (ppArrayEl = pArray->elements) + pArray->used;
		if((pArrayTarg = create(0, NULL, 0)) == NULL)
			return NULL;
		while(ppArrayEl < ppArrayElEnd) {
			if((*ppArrayEl)->type != dat_nil)
				if(put(pArrayTarg, pArrayTarg->used, *ppArrayEl, AOpCopy) != 0)
					return NULL;
			++ppArrayEl;
			}
		}

	return pArrayTarg;
	}

// Get an array element and return it, given signed index.  Return NULL if error.  If the index is negative, elements are
// selected from the end of the array such that the last element is -1, the second-to-last is -2, etc.; otherwise, elements are
// selected from the beginning with the first being 0.  If the index is zero or positive and the AOpGrow flag is set, the array
// will be enlarged if necessary to satisfy the request.  Any array elements that are created are set to nil values.  Otherwise,
// the referenced element must already exist.
Datum *aget(Array *pArray, ArraySize index, ushort aflags) {

	if(index < 0) {
		if(-index > pArray->used)
			goto NoSuch;
		index += pArray->used;
		}
	else if(!(aflags & AOpGrow)) {
		if(index >= pArray->used) {
NoSuch:
			emsgf(-1, "No such array element %ld (array size %ld)", index, pArray->used);
			return NULL;
			}
		}
	else if(need(pArray, 0, index, AOpFill) != 0)
		return NULL;

	return pArray->elements[index];
	}

// Create an array from a slice of given array and return it (or NULL if error), given signed index and length.  Length may be
// large to select all elements from index to end of array.  If AOpCut flag is set, transfer sliced elements to new array and
// close the gap; otherwise, copy them.
Array *aslice(Array *pArray, ArraySize index, ArraySize len, ushort aflags) {
	Array *pArray1;

	// If cutting elements, move them into new array; otherwise, copy them (to nil slots).
	if(normalize(pArray, &index, &len, AOpRemaining) != 0 ||
	 (pArray1 = create(len, NULL, aflags & AOpCut ? 0 : AOpFill)) == NULL)
		return NULL;
	if(len > 0) {
		Datum **ppArrayEl = pArray->elements + index;
		Datum **ppArrayElEnd = ppArrayEl + len;
		Datum **ppArrayEl1 = pArray1->elements;
		do {
			if(aflags & AOpCut)
				*ppArrayEl1 = *ppArrayEl;
			else if((*ppArrayEl)->type == dat_arrayRef && (aflags & AOpCloneAll)) {
				if(dsetarray((*ppArrayEl)->u.pArray, *ppArrayEl1) != 0)
					return NULL;
				}
			else if(dcpy(*ppArrayEl1, *ppArrayEl) != 0)
				return NULL;
			++ppArrayEl1;
			} while(++ppArrayEl < ppArrayElEnd);
		if(aflags & AOpCut)
			pArray1->used = len;
		}

	// Shrink original array if needed and return new array.
	if(aflags & AOpCut)
		shrink(pArray, index, len);
	return pArray1;
	}

// Clone an array and return the copy, or NULL if error.
Array *aclone(const Array *pArray) {
	Array *pArray1;

	if(pArray->used == 0)
		return create(0, NULL, 0);

	// Generate new array ID if at top-level.
	if(arrayNestLevel == 0)
		genArrayID();

	// Error if array contains itself.
	if(pArray->id == arrayID) {
		(void) emsg(-1, "Cannot clone array that contains itself");
		return NULL;
		}

	// Set array ID and clone array via aslice().
	++arrayNestLevel;
	((Array *) pArray)->id = arrayID;
	if((pArray1 = aslice((Array *) pArray, 0, pArray->used, AOpCloneAll)) == NULL) {
		--arrayNestLevel;
		return NULL;
		}

	--arrayNestLevel;
	return pArray1;
	}

// Replace elements in array slice with copies of *pVal object and return status code.  Allow slice to extend beyond end of
// array if AOpGrow flag is set.
int afill(Array *pArray, const Datum *pVal, ArraySize index, ArraySize len, ushort aflags) {

	if(normalize(pArray, &index, &len, aflags & AOpGrow) != 0)
		return -1;
	if(len > 0) {
		Datum **ppArrayEl, **ppArrayElEnd;

		if(index + len > pArray->used)
			if(need(pArray, 0, index + len - 1, AOpFill) != 0)
				return -1;
		ppArrayElEnd = (ppArrayEl = pArray->elements + index) + len;
		do {
			if(dcpy(*ppArrayEl++, pVal) != 0)
				return -1;
			} while(ppArrayEl < ppArrayElEnd);
		}

	return 0;
	}

// Remove a Datum object from an array at given index, shrink array by one, and return removed element.  If no elements left,
// return NULL.
static Datum *cut(Array *pArray, ArraySize index) {

	if(pArray->used == 0)
		return NULL;

	// Grab element, abandon slot, and return it.
	Datum *pDatum = pArray->elements[index];
	shrink(pArray, index, 1);
	return pDatum;
	}

// Remove a Datum object from end of an array, shrink array by one, and return removed element.  If no elements left,
// return NULL.
Datum *apop(Array *pArray) {

	return cut(pArray, pArray->used - 1);
	}

// Append a Datum object (or a copy if AOpCopy flag set) to an array.  Return status code.
int apush(Array *pArray, Datum *pVal, ushort aflags) {

	return put(pArray, pArray->used, pVal, aflags & AOpCopy);
	}

// Remove a Datum object from beginning of an array, shrink array by one, and return removed element.  If no elements left,
// return NULL.
Datum *ashift(Array *pArray) {

	return cut(pArray, 0);
	}

// Prepend a Datum object (or a copy if AOpCopy flag set) to an array.  Return status code.
int aunshift(Array *pArray, Datum *pVal, ushort aflags) {

	return put(pArray, 0, pVal, aflags & AOpCopy);
	}

// Remove a Datum object from an array at given signed index, shrink array by one, and return removed element.  If index out of
// range, set error and return NULL.
Datum *adelete(Array *pArray, ArraySize index) {
	ArraySize len = 1;

	return (normalize(pArray, &index, &len, 0) != 0) ? NULL : cut(pArray, index);
	}

// Insert a Datum object (or a copy if AOpCopy flag set) into an array, given signed index (which may be equal to "used" size)
// and return pointer to array, or NULL if error.  If index is non-negative, insert datum before the selected position; if index
// is negative, insert after.  Return status code.
int ainsert(Array *pArray, Datum *pVal, ArraySize index, ushort aflags) {
	ArraySize len = 1;
	ushort i = index < 0 ? 1 : 0;

	if(index != pArray->used && normalize(pArray, &index, &len, 0) != 0)
		return -1;
	return put(pArray, index + i, pVal, aflags & AOpCopy);
	}

// Step through an array, returning each element in sequence, or NULL if none left.  "ppArray" is an indirect pointer to the
// array object and is modified by this routine.
Datum *aeach(Array **ppArray) {
	static struct {
		Datum **ppArrayEl, **ppArrayElEnd;
		} arrayState = {
			NULL, NULL
			};

	// Validate and initialize control pointers.
	if(*ppArray != NULL) {
		if((*ppArray)->size == 0)
			goto Done;
		arrayState.ppArrayElEnd = (arrayState.ppArrayEl = (*ppArray)->elements) + (*ppArray)->used;
		*ppArray = NULL;
		}
	else if(arrayState.ppArrayEl == NULL)
		return NULL;

	// Get next element and return it, or NULL if none left.
	if(arrayState.ppArrayEl == arrayState.ppArrayElEnd) {
Done:
		arrayState.ppArrayEl = NULL;
		return NULL;
		}
	return *arrayState.ppArrayEl++;
	}

// Split a string into an array using given field delimiter and limit value.  Substrings are separated by a single character,
// white space, a null string, or no delimiter, as indicated by integer value "delim":
//	VALUE		MEANING
//	'\0'		Delimiter is a null string.  String is split into individual characters, including white space
//			characters, if any.
//	' '		Delimiter is white space.  One or more of the characters (' ', '\t', '\n', '\r', '\f', '\v') are treated
//			as a single delimiter.  Additionally, leading white space in the string is ignored.
//	1 - 0xFF	ASCII value of a character to use as the delimiter (except ' ').
//	< 0 or > 0xFF	No delimiter is defined.  A single-element array is returned containing the original string, unmodified.
//
// The "limit" argument controls the splitting process, as follows:
//	VALUE		MEANING
//	< 0		Each delimiter found is significant and delineates two substrings, either or both of which may be null.
//	0		Trailing null substrings are suppressed.
//	> 0		Maximum number of array elements to return.  The last element of the array will contain embedded
//			delimiter(s) if the number of delimiters in the string is equal to or greater than the "limit" value.
//
// Routine saves results in a new array and returns a pointer to the object, or NULL if error.
Array *asplit(short delim, const char *src, int limit) {
	Array *pArray;

	// Create an empty array.
	if((pArray = create(0, NULL, 0)) == NULL)
		return NULL;

	// Find tokens and push onto array.
	if(*src != '\0') {
		Datum *pDatum;
		const char *str0, *str1, *str;
		size_t len;
		int itemCount = 0;
		char delims[7];

		// Set actual delimiter string.
		if(delim == '\0' || delim < 0 || delim > 0xff)
			delims[0] = '\0';
		else if(delim != ' ') {
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

		// Scan string for delimiters, creating an element at the end of each loop, if applicable.
		for(;;) {
			// Save beginning position and find next delimiter.
			str0 = str;
			++itemCount;

			// Use all remaining characters as next element if limit reached or no delimiters left.
			if((limit > 0 && itemCount == limit) || (delim != '\0' && (str = strpbrk(str0, delims)) == NULL))
				len = (str = strchr(str0, '\0')) - str0;

			// Use current character as next element if delimiter is zero.
			else if(delim == '\0') {
				len = 1;
				++str;
				}

			// Found delimiter.  Check for run if delimter is white space or limit is zero.  Bail out or adjust str
			// pointer if needed.
			else {
				len = str - str0;
				if(delims[1] != '\0' || limit == 0)
					for(str1 = str; ; ++str1) {
						if(str1[1] == '\0') {

							// Hit delimiter at end of string.  If limit is zero, bail out if
							// delimiter run is entire string; otherwise, mark as last element.
							if(limit == 0) {
								if(len == 0)
									goto Done;
								str = str1 + 1;
								}
							else
								str = str1;
							break;
							}
						if(strchr(delims, str1[1]) == NULL) {

							// Not at end of string.  Ignore run if not white space.
							if(delims[1] != '\0')
								str = str1;
							break;
							}
						}
				}

			// Push substring onto array.
			if((pDatum = aget(pArray, pArray->used, AOpGrow)) == NULL || dsetsubstr(str0, len, pDatum) != 0)
				return NULL;

			// Onward.
			if(*str == '\0')
				break;
			if(delim != '\0')
				++str;
			}
		}
Done:
	return pArray;
	}

// Compare one array to another and return true if they are identical, otherwise false.  Ignore case in string comparisons if
// AOpIgnore flag set.  Also, treat an array that contains itself as unequal to any other.
bool aeq(const Array *pArray1, const Array *pArray2, ushort aflags) {
	ArraySize size = pArray1->used;

	// Do sanity checks.
	if(size != pArray2->used)
		return false;
	if(size == 0 || pArray1 == pArray2)
		return true;

	// Both arrays have at least one element and the same number.  Generate new array ID if at top level.
	if(arrayNestLevel == 0)
		genArrayID();

	// Check if either array contains itself (seen either array before?).  If detected, return false.
	if(pArray1->id == arrayID || pArray2->id == arrayID)
		return false;

	// We're good.  Save ID in both arrays and compare them.
	++arrayNestLevel;
	((Array *) pArray1)->id = ((Array *) pArray2)->id = arrayID;
	Datum **ppArrayEl1 = pArray1->elements;
	Datum **ppArrayEl2 = pArray2->elements;
	do {
		if(!deq(*ppArrayEl1++, *ppArrayEl2++, aflags & AOpIgnore)) {
			--arrayNestLevel;
			return false;
			}
		} while(--size > 0);

	--arrayNestLevel;
	return true;
	}

// Concatenate *pArray1 and *pArray2 into new array and return it (or graft *pArray2 onto *pArray1 if AOpInPlace flag is set and
// return pArray1).  Return NULL if error.
Array *acat(Array *pArray1, const Array *pArray2, ushort aflags) {
	Array *pArrayTarg;
	ArraySize used2;

	// Make copy of first array if not concatenating in place.
	if(aflags & AOpInPlace)
		pArrayTarg = pArray1;
	else if((pArrayTarg = aclone(pArray1)) == NULL)
		return NULL;

	// Append elements from second array, if any.
	if((used2 = pArray2->used) > 0) {
		Datum **ppSrcEl, **ppDestEl;
		ArraySize used0 = pArrayTarg->used;

		if(need(pArrayTarg, used2, -1, AOpFill) != 0)
			return NULL;
		ppDestEl = pArrayTarg->elements + used0;
		ppSrcEl = pArray2->elements;
		do {
			if(dcpy(*ppDestEl++, *ppSrcEl++) != 0)
				return NULL;
			} while(--used2 > 0);
		}

	return pArrayTarg;
	}

// Set intersection: return new array (or pArray1 if AOpInPlace flag set) of unique elements common to *pArray1 and *pArray2, or
// if AOpNonMatching flag set, unique elements from *pArray1 that do not occur in *pArray2.  If AOpIgnore flag set, ignore case
// in string comparisons.
Array *amatch(Array *pArray1, const Array *pArray2, ushort aflags) {
	Array *pArray;
	Datum **ppArrayEl0, **ppArrayEl, **ppArrayElEnd;
	bool match = (aflags & AOpNonMatching) == 0;

	if(aflags & AOpInPlace) {
		if((pArray = pArray1)->used > 0) {

			// Step through elements in first array.
			ppArrayElEnd = (ppArrayEl0 = ppArrayEl = pArray->elements) + pArray->used;
			do {
				if((scan(pArray2, pArray2->used, *ppArrayEl,	// Failed match or non-match with second array?
				 aflags & AOpIgnore) >= 0) != match)
					goto Delete;				// Yes, delete element.
				if(scan(pArray, ppArrayEl0 - pArray->elements,	// No, duplicate?
				 *ppArrayEl, aflags & AOpIgnore) >= 0)
Delete:
					dfree(*ppArrayEl);			// Yes, delete element.
				else if(ppArrayEl == ppArrayEl0)
					++ppArrayEl0;				// No, skip over it...
				else
					*ppArrayEl0++ = *ppArrayEl;		// or shift it left.
				} while(++ppArrayEl < ppArrayElEnd);

			pArray->used = ppArrayEl0 - pArray->elements;		// Update "used" length.
			}
		}
	else {
		if((pArray = create(0, NULL, 0)) == NULL)
			return NULL;
		if(pArray1->used > 0 && (pArray2->used > 0 || (aflags & AOpNonMatching))) {

			// Step through elements in first array.
			ppArrayElEnd = (ppArrayEl = pArray1->elements) + pArray1->used;
			do {
				if((scan(pArray2, pArray2->used, *ppArrayEl, aflags & AOpIgnore) >= 0) == match)
					if(scan(pArray, pArray->used, *ppArrayEl, aflags & AOpIgnore) < 0)
						if(put(pArray, pArray->used, *ppArrayEl, AOpCopy) != 0)
							return NULL;
				} while(++ppArrayEl < ppArrayElEnd);
			}
		}

	return pArray;
	}

// Set union: return new array (or pArray1 if AOpInPlace flag set) of unique elements from *pArray1 and, if pArray2 not NULL,
// *pArray2.  If AOpIgnore flag set, ignore case in string comparisons.
Array *auniq(Array *pArray1, const Array *pArray2, ushort aflags) {
	Array *pArray;
	Datum **ppArrayEl0, **ppArrayEl, **ppArrayElEnd;

	if(aflags & AOpInPlace) {
		if((pArray = pArray1)->used > 0) {

			// Step through elements in first array.
			ppArrayElEnd = (ppArrayEl0 = ppArrayEl = pArray->elements) + pArray->used;
			do {
				if(scan(pArray, ppArrayEl0 - pArray->elements,
				 *ppArrayEl, aflags & AOpIgnore) >= 0)		// Duplicate?
					dfree(*ppArrayEl);			// Yes, delete element.
				else if(ppArrayEl == ppArrayEl0)
					++ppArrayEl0;				// No, skip over it...
				else
					*ppArrayEl0++ = *ppArrayEl;		// or shift it left.
				} while(++ppArrayEl < ppArrayElEnd);

			pArray->used = ppArrayEl0 - pArray->elements;		// Update "used" length.
			}
		goto Array2;
		}
	else {
		if((pArray = create(0, NULL, 0)) == NULL)
			return NULL;

		// Step through elements in first array.
		ppArrayElEnd = (ppArrayEl = pArray1->elements) + pArray1->used;
		while(ppArrayEl < ppArrayElEnd) {
			if(scan(pArray, pArray->used, *ppArrayEl, aflags & AOpIgnore) < 0)
				if(put(pArray, pArray->used, *ppArrayEl, AOpCopy) != 0)
					return NULL;
			++ppArrayEl;
			}

Array2:
		if(pArray2 != NULL) {

			// Step through elements in second array.
			ppArrayElEnd = (ppArrayEl = pArray2->elements) + pArray2->used;
			while(ppArrayEl < ppArrayElEnd) {
				if(scan(pArray, pArray->used, *ppArrayEl, aflags & AOpIgnore) < 0)
					if(put(pArray, pArray->used, *ppArrayEl, AOpCopy) != 0)
						return NULL;
				++ppArrayEl;
				}
			}
		}

	return pArray;
	}

// Write an array in string form to a fabrication object per cflags, via calls to dputd(), and return status code.  If ACvtBrkts
// flag is set and ACvtNoBrkts flag is not set, array is written in "[...]" form; otherwise, brackets are omitted.  Elements are
// separated by "delim" delimiters if delim not NULL, otherwise commas if ACvtDelim flag set, otherwise a null string.  Array
// elements are converted via calls to dputd(), except that nil elements are skipped if CvtSkipNil flag is set.  In all cases,
// if the array includes itself, recursion stops, "[...]" (or "..." if no brackets) is written for the nested occurrence, and 1
// is returned instead of zero.
int aput(const Array *pArray, DFab *pFab, const char *delim, ushort cflags) {
	int rtnCode = 0;

	// Generate new array ID if at top-level.
	if(arrayNestLevel == 0)
		genArrayID();
	++arrayNestLevel;

	// Check array's ID, which should be a junk/random number if this is the first visit.  If it matches "arrayID" however,
	// then it is extremely likely that we've seen this array before; that is, the array includes itself, either directly or
	// indirectly via a nested array.  In that case, we need to stop the madness.
	if(pArray->id == arrayID) {

		// Yes.  Store an ellipsis and stop recursion here.
		if(bputs((cflags & (ACvtBrkts | ACvtNoBrkts)) == ACvtBrkts ? "[...]" : "...", pFab) != 0)
			goto ErrRetn;
		rtnCode = 1;
		}
	else {
		int r;
		Datum *pDatum;
		Datum **ppArrayEl = pArray->elements;
		ArraySize n = pArray->used;
		bool first = true;
		const char *realDelim = delim != NULL ? delim : cflags & ACvtDelim ? ", " : NULL;

		((Array *) pArray)->id = arrayID;
		if((cflags & (ACvtBrkts | ACvtNoBrkts)) == ACvtBrkts && bputc('[', pFab) != 0)
			goto ErrRetn;

		while(n-- > 0) {
			pDatum = *ppArrayEl++;

			// Skip nil value if requested.
			if(pDatum->type == dat_nil && (cflags & ACvtSkipNil))
				continue;

			// Write optional delimiter and encoded value.
			if(!first && realDelim != NULL && bputs(realDelim, pFab) != 0)
				goto ErrRetn;
			if((r = dputd(pDatum, pFab, delim, cflags)) < 0)
				goto ErrRetn;
			if(r > 0)
				rtnCode = r;
			first = false;
			}
		if((cflags & (ACvtBrkts | ACvtNoBrkts)) == ACvtBrkts && bputc(']', pFab) != 0)
			goto ErrRetn;
		}

	--arrayNestLevel;
	return rtnCode;
ErrRetn:
	--arrayNestLevel;
	return -1;
	}

// Join all array elements in string form into *pDatum per cflags with given string delimiter.  If successful, return 0 (or 1 if
// endless array recursion detected), otherwise -1 (error).
int ajoin(Datum *pDatum, const Array *pArray, const char *delim, ushort cflags) {
	int rtnCode = 0;

	if(pArray->used == 0)
		dsetnull(pDatum);
	else if(pArray->used == 1)
		rtnCode = dtos(pDatum, pArray->elements[0], delim, cflags);
	else {
		DFab fab;

		if(dopenwith(&fab, pDatum, FabClear) != 0 || (rtnCode = aput(pArray, &fab, delim, cflags | ACvtNoBrkts)) < 0 ||
		 dclose(&fab, FabStr) != 0)
			return -1;
		}
	return rtnCode;
	}

// Convert array in *pArray to string per cflags and store in *pDatum.  Return pDatum, or NULL if error.
int atos(Datum *pDatum, const Array *pArray, const char *delim, ushort cflags) {
	DFab fab;
	int rtnCode = 0;

	return dopenwith(&fab, pDatum, FabClear) != 0 || (rtnCode = aput(pArray, &fab, delim, cflags)) < 0 ||
	 dclose(&fab, FabStr) != 0 ? -1 : rtnCode;
	}
