// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// array.c		Array-handling routines.

#include "stdos.h"
#include "cxl/excep.h"
#include "cxl/datum.h"
#include "cxl/array.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>

// Initialize an Array object as an empty array and return pointer to it.
Array *ainit(Array *pArray) {

	pArray->used = pArray->size = 0;
	pArray->elements = NULL;
	return pArray;
	}

// Clear an array: free element storage (if any), set used size to zero, and return a pointer to the reinitialized Array object
// (to be freed or reused by the caller).
Array *aclear(Array *pArray) {
	Datum **ppArrayEl = pArray->elements;
	if(ppArrayEl != NULL) {
		Datum **ppArrayElEnd = ppArrayEl + pArray->used;
		while(ppArrayEl < ppArrayElEnd)
			ddelete(*ppArrayEl++);
		free((void *) pArray->elements);
		}
	return ainit(pArray);
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
// double old size for second and third, then increase old size by 7/8 thereafter (to prevent the array from growing too fast
// for large numbers).  Return error if size needed exceeds maximum.  Any slots allocated beyond the current "used" size are
// left undefined if "fill" is false; otherwise, new slots up to and including the index are set to nil and the "used" size is
// updated.  It is assumed that only one of "growSize" or "index" will be valid, so only one is checked.
static int aneed(Array *pArray, ArraySize growSize, ArraySize index, bool fill) {
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
		if(index == ArraySizeMax || pArray->size == ArraySizeMax) // Need more space.  Error if request exceeds maximum.
TooMuch:
			return emsgf(-1, "Cannot grow array beyond maximum size (%d)", ArraySizeMax);
		minSize = index + 1;
		}

	// Expansion needed.  Double old size until it's big enough, without causing an integer overflow.
	ArraySize newSize = pArray->size;
	do {
		newSize = (newSize == 0) ? ArrayChunkSize : (newSize < ArrayChunkSize * 4) ? newSize * 2 :
		 (ArraySizeMax - newSize < newSize) ? ArraySizeMax : newSize + (newSize >> 3) * 7;
		} while(newSize < minSize);

	// Get more space.
	if((pArray->elements = (Datum **) realloc((void *) pArray->elements, newSize * sizeof(Datum *))) == NULL) {
		cxlExcep.flags |= ExcepMem;
		return emsgf(-1, "%s, allocating %d-element array", strerror(errno), newSize);
		}
	pArray->size = newSize;
CheckFill:
	if(fill) {
		// Set new elements to nil and increase "used" size.
		if(plugNil(pArray, pArray->used, growSize) != 0)
			return -1;
		pArray->used += growSize;
		}

	return 0;
	}

// Validate and normalize array slice parameters (convert to non-negative values).  Return status code.
static int normalize(Array *pArray, ArraySize *pIndex, ArraySize *pLen, bool allowNegLen, bool allowBigLen) {
	ArraySize index = *pIndex;
	ArraySize len = *pLen;

	// Validate and normalize index.
	if(index < 0) {
		if(-index > pArray->used)
			goto IndexErr;
		index += pArray->used;
		}
	else if(index >= pArray->used)
IndexErr:
		return emsgf(-1, "Array index %d out of range (array size %d)", index, pArray->used);

	// Validate and normalize length.
	if(len < 0) {
		if(!allowNegLen || -len > pArray->used)
			goto RangeErr;
		ArraySize index1 = len + pArray->used;
		if(index1 < index)
			goto RangeErr;
		len = index1 - index;
		}

	// Validate slice span.
	if(index + len > pArray->used) {
		if(allowBigLen)
			len = pArray->used - index;
		else
RangeErr:
			return emsgf(-1, "Array slice values [%d,%d] out of range (array size %d)",
			 *pIndex, *pLen, pArray->used);
		}

	// All is well.  Return normalized slice values.
	*pIndex = index;
	*pLen = len;
	return 0;
	}

// Open slot in an array at given index by shifting higher elements to the right one position (so that a value can be inserted
// there by the caller).  If "plugSlot" is true, set slot to a nil value.  Update "used" size and return status code.  Index is
// assumed to be <= "used" size.
static int aspread(Array *pArray, ArraySize index, bool plugSlot) {

	if(aneed(pArray, 1, -1, false) != 0)			// Ensure a slot exists at end of used portion.
		return -1;
	if(index < pArray->used) {				// If not inserting at end of array...
		Datum **ppArrayEl0, **ppArrayEl1;

		// 012345678	012345678	index = 1
		// abcdef...	a.bcdef..
		//  0    1
		ppArrayEl0 = pArray->elements + index;		// set slot pointers...
		ppArrayEl1 = pArray->elements + pArray->used;
		do {						// and shift elements to the right.
			ppArrayEl1[0] = ppArrayEl1[-1];
			} while(--ppArrayEl1 > ppArrayEl0);
		}
	++pArray->used;						// Bump "used" size...
	return plugSlot ? plugNil(pArray, index, 1) : 0;	// and set slot to nil if requested.
	}

// Remove one or more contiguous elements from an array at given location.  It is assumed that the Datum object(s) in the slice
// have already been saved or freed and that "index" and "len" are in range.
static void ashrink(Array *pArray, ArraySize index, ArraySize len) {

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

// Remove nil elements from an array (in place) and shift elements left to fill the hole(s).
void acompact(Array *pArray) {

	if(pArray->used > 0) {
		Datum **ppArrayEl0, **ppArrayEl1, **ppArrayElEnd;
		ppArrayElEnd = (ppArrayEl0 = ppArrayEl1 = pArray->elements) + pArray->used;
		do {
			if((*ppArrayEl1)->type == dat_nil)		// Nil element?
				ddelete(*ppArrayEl1);			// Yes, delete it.
			else
				*ppArrayEl0++ = *ppArrayEl1;		// No, shift it left.
			} while(++ppArrayEl1 < ppArrayElEnd);
		pArray->used = ppArrayEl0 - pArray->elements;		// Update "used" length.
		}
	}

// Get an array element and return it, given signed index.  Return NULL if error.  If the index is negative, elements are
// selected from the end of the array such that the last element is -1, the second-to-last is -2, etc.; otherwise, elements are
// selected from the beginning with the first being 0.  If the index is zero or positive and "force" is true, the array will be
// enlarged if necessary to satisfy the request.  Any array elements that are created are set to nil values.  Otherwise, the
// referenced element must already exist.
Datum *aget(Array *pArray, ArraySize index, bool force) {

	if(index < 0) {
		if(-index > pArray->used)
			goto NoSuch;
		index += pArray->used;
		}
	else if(!force) {
		if(index >= pArray->used) {
NoSuch:
			emsgf(-1, "No such array element %d (array size %d)", index, pArray->used);
			return NULL;
			}
		}
	else if(aneed(pArray, 0, index, true) != 0)
		return NULL;

	return pArray->elements[index];
	}

// Create an array (in heap space) and return pointer to it, or NULL if error.  If len > 0, pre-allocate that number of nil (or
// *pVal) elements; otherwise, leave empty.
Array *anew(ArraySize len, const Datum *pVal) {
	Array *pArray;

	if(len < 0) {
		emsgf(-1, "Invalid array length (%d)", len);
		goto ErrRtn;
		}

	if((pArray = (Array *) malloc(sizeof(Array))) == NULL) {	// Get space for array object...
		cxlExcep.flags |= ExcepMem;
		emsgsys(-1);
		goto ErrRtn;
		}
	ainit(pArray);							// initialize empty array...
	if(len > 0) {							// allocate elements if requested...
		if(aneed(pArray, 0, len - 1, true) != 0)
			goto ErrRtn;
		if(pVal != NULL) {
			Datum **ppArrayEl = pArray->elements;
			do {
				if(datcpy(*ppArrayEl++, pVal) != 0)
					goto ErrRtn;
				} while(--len > 0);
			}
		}
	return pArray;							// and return it.
ErrRtn:
	return NULL;
	}

// Create an array from a slice of another and return it (or NULL if error), given signed index and length.  If "force" is true,
// allow length to exceed number of elements in array beyond index.  If "cut" is true, delete sliced elements from source array.
Array *aslice(Array *pArray, ArraySize index, ArraySize len, bool force, bool cut) {
	Array *pArray1;

	if(normalize(pArray, &index, &len, true, force) != 0 || (pArray1 = anew(cut ? 0 : len, NULL)) == NULL)
		return NULL;
	if(len > 0) {
		if(cut) {
			if(aneed(pArray1, len, -1, false) != 0)
				return NULL;
			pArray1->used = len;
			}
		Datum **ppArrayEl = pArray->elements + index;
		Datum **ppArrayElEnd = ppArrayEl + len;
		Datum **ppArrayEl1 = pArray1->elements;
		do {
			if(cut)
				*ppArrayEl1 = *ppArrayEl;
			else if(datcpy(*ppArrayEl1, *ppArrayEl) != 0)
				return NULL;
			++ppArrayEl1;
			} while(++ppArrayEl < ppArrayElEnd);
		}

	// Shrink original array if needed and return new array.
	if(cut)
		ashrink(pArray, index, len);
	return pArray1;
	}

// Clone an array and return the copy, or NULL if error.
Array *aclone(Array *pArray) {

	return pArray->used == 0 ? anew(0, NULL) : aslice(pArray, 0, pArray->used, false, false);
	}

// Remove a Datum object from an array at given index, shrink array by one, and return removed element.  If no elements left,
// return NULL.
static Datum *acut(Array *pArray, ArraySize index) {

	if(pArray->used == 0)
		return NULL;

	// Grab element, abandon slot, and return it.
	Datum *pDatum = pArray->elements[index];
	ashrink(pArray, index, 1);
	return pDatum;
	}

// Insert a Datum object (or a copy if "copy" is true) into an array.  Return status code.
static int aput(Array *pArray, ArraySize index, Datum *pDatum, bool copy) {

	if(aspread(pArray, index, copy) != 0)
		return -1;
	if(!copy)
		pArray->elements[index] = pDatum;
	else if(datcpy(pArray->elements[index], (const Datum *) pDatum) != 0)
		return -1;
	return 0;
	}

// Remove a Datum object from end of an array, shrink array by one, and return removed element.  If no elements left,
// return NULL.
Datum *apop(Array *pArray) {

	return acut(pArray, pArray->used - 1);
	}

// Append a Datum object (or a copy if "copy" is true) to an array.  Return status code.
int apush(Array *pArray, Datum *pVal, bool copy) {

	return aput(pArray, pArray->used, pVal, copy);
	}

// Remove a Datum object from beginning of an array, shrink array by one, and return removed element.  If no elements left,
// return NULL.
Datum *ashift(Array *pArray) {

	return acut(pArray, 0);
	}

// Prepend a Datum object (or a copy if "copy" is true) to an array.  Return status code.
int aunshift(Array *pArray, Datum *pVal, bool copy) {

	return aput(pArray, 0, pVal, copy);
	}

// Remove a Datum object from an array at given signed index, shrink array by one, and return removed element.  If index out of
// range, set error and return NULL.
Datum *adelete(Array *pArray, ArraySize index) {
	ArraySize len = 1;

	return (normalize(pArray, &index, &len, false, false) != 0) ? NULL : acut(pArray, index);
	}

// Insert a Datum object (or a copy if "copy" is true) into an array, given signed index (which may be equal to "used" size).
// Return status code.
int ainsert(Array *pArray, ArraySize index, Datum *pVal, bool copy) {
	ArraySize len = 1;

	if(index == pArray->used)
		return apush(pArray, pVal, copy);
	if(normalize(pArray, &index, &len, false, false) != 0)
		return -1;
	return aput(pArray, index, pVal, copy);
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

// Join all array elements into pDest with given string delimiter.  Return status code.
int ajoin(Datum *pDest, const Array *pArray, const char *delim) {
	char *str;

	if(pArray->used == 0)
		dsetnull(pDest);
	else if(pArray->used == 1) {
		if((str = dtos(pArray->elements[0], false)) == NULL || dsetstr(str, pDest) != 0)
			return -1;
		}
	else {
		DFab fab;
		Datum **ppArrayEl = pArray->elements;
		ArraySize n = pArray->used;

		if(dopenwith(&fab, pDest, false) != 0)
			return -1;
		for(;;) {
			if((str = dtos(*ppArrayEl++, false)) == NULL || dputs(str, &fab) != 0)
				return -1;
			if(--n == 0)
				break;
			if(dputs(delim, &fab) != 0)
				return -1;
			}
		if(dclose(&fab, Fab_String) != 0)
			return -1;
		}
	return 0;
	}

// Split a string into an array using given field delimiter and limit value.  Substrings are separated by a single character,
// white space, or no delimiter, as indicated by integer value "delim":
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
// Routine saves results in a new array and returns a pointer to the object, or NULL if error.
Array *asplit(short delim, const char *src, int limit) {
	Array *pArray;

	// Create an empty array.
	if((pArray = anew(0, NULL)) == NULL)
		return NULL;

	// Find tokens and push onto array.
	if(*src != '\0') {
		Datum *pDatum;
		const char *str0, *str1, *str;
		size_t len;
		int itemCount = 0;
		char delims[7];

		// Set actual delimiter string.
		if(delim > 0xff)
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

		// Scan string for delimiters, creating an element at the end of each loop, if applicable.
		for(;;) {
			// Save beginning position and find next delimiter.
			str0 = str;

			// Use all remaining characters as next element if limit reached or no delimiters left.
			++itemCount;
			if((limit > 0 && itemCount == limit) || (str = strpbrk(str0, delims)) == NULL)
				len = (str = strchr(str0, '\0')) - str0;

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
			if((pDatum = aget(pArray, pArray->used, true)) == NULL || dsetsubstr(str0, len, pDatum) != 0)
				return NULL;

			// Onward.
			if(*str++ == '\0')
				break;
			}
		}
Done:
	return pArray;
	}

// Compare one array to another and return true if they are identical; otherwise, false.
bool aeq(const Array *pArray1, const Array *pArray2, bool ignore) {
	ArraySize size = pArray1->used;

	// Do sanity checks.
	if(size != pArray2->used)
		return false;
	if(size == 0)
		return true;

	// Both arrays have at least one element and the same number.  Compare them.
	Datum **ppArrayEl1 = pArray1->elements;
	Datum **ppArrayEl2 = pArray2->elements;
	do {
		if(!dateq(*ppArrayEl1++, *ppArrayEl2++, ignore))
			return false;
		} while(--size > 0);

	return true;
	}

// Graft pArray2 onto pArray1 (by copying elements) and return pArray1, or NULL if error.
Array *agraft(Array *pArray1, const Array *pArray2) {
	ArraySize used2;

	if((used2 = pArray2->used) > 0) {
		Datum **ppSrcEl, **ppDestEl;
		ArraySize used1 = pArray1->used;

		if(aneed(pArray1, used2, -1, true) != 0)
			return NULL;
		ppDestEl = pArray1->elements + used1;
		ppSrcEl = pArray2->elements;
		do {
			if(datcpy(*ppDestEl++, *ppSrcEl++) != 0)
				return NULL;
			} while(--used2 > 0);
		}

	return pArray1;
	}
