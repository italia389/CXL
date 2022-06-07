// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// datum.c		Datum object routines.

#include "stdos.h"
#include "cxl/excep.h"
#include "cxl/datum.h"
#include "cxl/array.h"
#include "cxl/string.h"
#include "cxl/lib.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

// Local definitions.
#define ChunkSize0	64			// Starting size of fabrication chunks/pieces.
#define ChunkSize4	512			// Size at which to begin quadrupling until hit maximum.
#define ChunkSizeMax	32768			// Maximum size (32K).

// Global variables.
Datum *datGarbHead = NULL;			// Head of list of temporary Datum objects, for "garbage collection".

// Initialize a Datum object to nil.  It is assumed that any allocated memory has already been freed or datum is a
// local variable.
void dinit(Datum *pDatum) {

	pDatum->type = dat_nil;
	pDatum->str = NULL;
	}

// Clear a Datum object and set it to nil.
void dclear(Datum *pDatum) {

	// Free any string, byte string, or array storage.
	switch(pDatum->type) {
		case dat_longStr:
			free((void *) pDatum->str);
			break;
		case dat_byteStr:
			if(pDatum->u.mem.ptr != NULL)
				free((void *) pDatum->u.mem.ptr);
			break;
		case dat_array:
			afree(pDatum->u.pArray);
		}
	dinit(pDatum);
	}

// Free memory for given Datum object; that is, clear (free) its contents and free the object.  It assumed that either the
// object was permanent and not in the garbage collection list, or the caller is removing it from the list; for example, when
// called from dgarbpop().
void dfree(Datum *pDatum) {

	dclear(pDatum);
	free((void *) pDatum);
	}

// Change a datum with allocated data to the corresponding reference type, if possible, and return status code.
int drelease(Datum *pDatum) {

	switch(pDatum->type) {
		case dat_miniStr:
			return emsgf(-1, "drelease(): String \"%s\" was not allocated", pDatum->str);
		case dat_longStr:
			pDatum->type = dat_longStrRef;
			break;
		case dat_byteStr:
			pDatum->type = dat_byteStrRef;
			break;
		case dat_array:
			pDatum->type = dat_arrayRef;
			break;
		}
	return 0;
	}

// Pop datGarbHead to given pointer, releasing heap space and laughing all the way.
void dgarbpop(const Datum *pDatum) {
	Datum *pDatum1;

	while(datGarbHead != pDatum) {
		pDatum1 = datGarbHead;
		datGarbHead = datGarbHead->next;
		dfree(pDatum1);
		}
	}

// Set a Datum object to a null "mini" string.
void dsetnull(Datum *pDatum) {

	dclear(pDatum);
	*(pDatum->str = pDatum->u.miniStr) = '\0';
	pDatum->type = dat_miniStr;
	}

// Set a Boolean value in a Datum object.
void dsetbool(bool b, Datum *pDatum) {

	dclear(pDatum);
	pDatum->type = b ? dat_true : dat_false;
	}

// Set a memory value in a Datum object (copy to heap), given memory pointer to object and size in bytes, which may be zero.
// Return status code.
int dsetmem(const void *memPtr, size_t size, Datum *pDatum) {
	DMem *pMem = &pDatum->u.mem;

	dclear(pDatum);
	pDatum->type = dat_byteStr;
	if((pMem->size = size) == 0)
		pMem->ptr = NULL;
	else {
		if((pMem->ptr = malloc(size)) == NULL) {
			cxlExcep.flags |= ExcepMem;
			return emsgsys(-1);
			}
		memcpy(pMem->ptr, memPtr, size);
		}

	return 0;
	}

// Set a byte string in a Datum object of given type.
static void setMemRef(void *memPtr, size_t size, Datum *pDatum, DatumType type) {
	DMem *pMem = &pDatum->u.mem;

	dclear(pDatum);
	pMem->ptr = memPtr;
	pMem->size = size;
	pDatum->type = type;
	}

// Set a memory reference in a Datum object, given memory pointer to object and size in bytes.
void dsetmemref(void *memPtr, size_t size, Datum *pDatum) {

	setMemRef(memPtr, size, pDatum, dat_byteStrRef);
	}

// Store a pointer to a byte string currently allocated in memory into a Datum object; that is, "adopt" the byte string and free
// it when the Datum object is cleared.
void dadoptmem(void *memPtr, size_t size, Datum *pDatum) {

	setMemRef(memPtr, size, pDatum, dat_byteStr);
	}

// Set an array in a Datum object (clone array), given array pointer.  Return status code.
int dsetarray(const Array *pArray, Datum *pDatum) {
	Array *pArray1;

	dclear(pDatum);
	if((pArray1 = aclone(pArray)) == NULL)
		return -1;
	pDatum->type = dat_array;
	pDatum->u.pArray = pArray1;
	return 0;
	}

// Set an array in a Datum object of given type.
static void setArrayRef(Array *pArray, Datum *pDatum, DatumType type) {

	dclear(pDatum);
	pDatum->u.pArray = pArray;
	pDatum->type = type;
	}

// Set an array reference in a Datum object, given array pointer.
void dsetarrayref(Array *pArray, Datum *pDatum) {

	setArrayRef(pArray, pDatum, dat_arrayRef);
	}

// Store a pointer to an array currently allocated in memory into a Datum object; that is, "adopt" the array and free it when
// the Datum object is cleared.
void dadoptarray(Array *pArray, Datum *pDatum) {

	setArrayRef(pArray, pDatum, dat_array);
	}

// Set a single-character value in a Datum object.
void dsetchr(short c, Datum *pDatum) {

	dclear(pDatum);
	pDatum->u.c = c;
	pDatum->type = dat_char;
	}

// Set a signed integer value in a Datum object.
void dsetint(long i, Datum *pDatum) {

	dclear(pDatum);
	pDatum->u.intNum = i;
	pDatum->type = dat_int;
	}

// Set an unsigned integer value in a Datum object.
void dsetuint(ulong u, Datum *pDatum) {

	dclear(pDatum);
	pDatum->u.uintNum = u;
	pDatum->type = dat_uint;
	}

// Set a real number value in a Datum object.
void dsetreal(double d, Datum *pDatum) {

	dclear(pDatum);
	pDatum->u.realNum = d;
	pDatum->type = dat_real;
	}

// Get space for a string and set *pStr to it.  If a mini string will work, use caller's mini buffer 'miniBuf' (if not NULL).
// Return status code.
static int salloc(char **pStr, size_t len, char *miniBuf) {

	if(miniBuf != NULL && len <= sizeof(DMem))
		*pStr = miniBuf;
	else if((*pStr = (char *) malloc(len)) == NULL) {
		cxlExcep.flags |= ExcepMem;
		return emsgsys(-1);
		}
	return 0;
	}

// Allocate a string value of given size in a Datum object.  Return status code.
int dsalloc(Datum *pDatum, size_t len) {

	dsetnull(pDatum);
	if(len > sizeof(DMem)) {
		if(salloc(&pDatum->u.longStr, len, NULL) != 0)
			return -1;
		*(pDatum->str = pDatum->u.longStr) = '\0';
		pDatum->type = dat_longStr;
		}
	return 0;
	}

// Set a string in a Datum object of given type.
static void setStrRef(char *str, Datum *pDatum, DatumType type) {

	dclear(pDatum);
	pDatum->str = pDatum->u.longStr = str;
	pDatum->type = type;
	}

// Store a pointer to a string currently allocated in memory into a Datum object; that is, "adopt" the string and free it when
// the Datum object is cleared.
void dadoptstr(char *str, Datum *pDatum) {

	setStrRef(str, pDatum, dat_longStr);
	}

// Set a string reference in a Datum object.
void dsetstrref(char *str, Datum *pDatum) {

	setStrRef(str, pDatum, dat_longStrRef);
	}

// Set a substring (up to "len" characters) in a Datum object.  Routine assumes that the source string may be all or part of the
// dest object.  Return status code.
int dsetsubstr(const char *str, size_t len, Datum *pDatum) {
	char *str0;

	if(dtypstr(pDatum) && str >= pDatum->str) {		// If string overlap possible...
		char workBuf[sizeof(DMem)];

		if(salloc(&str0, len + 1, workBuf) != 0)	// Get space for string if not a mini...
			return -1;
		stplcpy(str0, str, len + 1);			// and copy string to new buffer.
		if(str0 == workBuf) {				// If mini string...
			dsetnull(pDatum);			// clear datum...
			strcpy(pDatum->u.miniStr, workBuf);	// and copy string to it.
			}
		else
			goto Adopt;				// Otherwise, set allocated string in pDatum.
		}
	else if(len < sizeof(DMem)) {				// No overlap.  If string fits in mini string...
		dsetnull(pDatum);				// clear datum...
		stplcpy(pDatum->u.miniStr, str, len + 1);	// and copy string to mini buffer.
		}
	else {							// String won't fit.
		if(salloc(&str0, len + 1, NULL) != 0)		// Get space for string...
			return -1;
		stplcpy(str0, str, len + 1);			// copy string to memory buffer...
Adopt:
		dadoptstr(str0, pDatum);			// and set allocated string in datum.
		}

	return 0;
	}

// Set a string value in a Datum object.  Return status code.
int dsetstr(const char *str, Datum *pDatum) {

	return dsetsubstr(str, strlen(str), pDatum);
	}

// Convert a character to a string and store in a datum.
void dconvchr(short c, Datum *pDatum) {

	dsetnull(pDatum);
	pDatum->u.miniStr[0] = c;
	pDatum->u.miniStr[1] = '\0';
	}

// Transfer contents of one Datum object to another.
void dxfer(Datum *pDest, Datum *pSrc) {
	Datum *pDatum = pDest->next;			// Save the "next" pointer...
	dclear(pDest);					// free dest...
	*pDest = *pSrc;					// copy the whole burrito...
	pDest->next = pDatum;				// restore "next" pointer...
	if(pDest->type == dat_miniStr)			// fix mini-string pointer...
		pDest->str = pDest->u.miniStr;
	dinit(pSrc);					// and initialize the source (discard contents).
	}

// Return true if a Datum object is a null string, otherwise false.
bool disnull(const Datum *pDatum) {

	return dtypstr(pDatum) && *pDatum->str == '\0';
	}

// Return true if a Datum object is empty (is nil, a null string, or a zero-element array), otherwise false.
bool disempty(const Datum *pDatum) {

	return disnil(pDatum) || disnull(pDatum) || (dtyparray(pDatum) && pDatum->u.pArray->used == 0);
	}

// Create Datum object and set *ppDatum to it.  If 'track' is true, add it to the garbage collection stack.  Return status code.
static int dmake(Datum **ppDatum, bool track) {
	Datum *pDatum;

	// Get new object...
	if((pDatum = (Datum *) malloc(sizeof(Datum))) == NULL) {
		cxlExcep.flags |= ExcepMem;
		return emsgsys(-1);
		}

	// add it to garbage collection stack (if applicable)...
	if(track) {
		pDatum->next = datGarbHead;
		datGarbHead = pDatum;
		}
	else
		pDatum->next = NULL;

	// initialize it...
	dinit(pDatum);

	// and return the spoils.
	*ppDatum = pDatum;
	return 0;
	}

// Create Datum object and set *ppDatum to it.  Object is not added it to the garbage collection stack.  Return status code.
int dnew(Datum **ppDatum) {

	return dmake(ppDatum, false);
	}

// Create (tracked) Datum object, set *ppDatum to it, and add it to the garbage collection stack.  Return status code.
int dnewtrack(Datum **ppDatum) {

	return dmake(ppDatum, true);
	}

// Copy a byte string into a fabrication object's work buffer, left or right justified.  Source string may already be in the
// buffer.
static void fabCopy(DFab *pFab, const char *str, size_t len) {

	if(len > 0) {
		if((pFab->flags & FabModeMask) != FabPrepend) {

			// Appending to existing byte string: copy it into the work buffer.
			pFab->bufCur = (char *) memstpcpy((void *) pFab->buf, (void *) str, len);
			}
		else {
			// Prepending to existing byte string or right-shifting the data in the work buffer.
			char *dest = pFab->bufEnd;
			str += len;
			do {
				*--dest = *--str;
				} while(--len > 0);
			pFab->bufCur = dest;
			}
		}
	}

// Save a byte string to a fabrication object's chunk list.  Return status code.
static int fabSave(char *str, size_t len, DFab *pFab) {
	DChunk *pChunk;

	// Get new chunk...
	if((pChunk = (DChunk *) malloc(sizeof(DChunk))) == NULL) {
		cxlExcep.flags |= ExcepMem;
		return emsgsys(-1);
		}
	pChunk->mem.ptr = (void *) str;
	pChunk->mem.size = len;

	// and push it onto the stack in the fabrication object.
	pChunk->next = pFab->stack;
	pFab->stack = pChunk;

	return 0;
	}

// Grow work buffer in a fabrication object.  Assume minSize is less than ChunkSizeMax.  If not initial allocation and
// prepending, the data in the work buffer is shifted right after the work buffer is extended.  Return status code.
static int fabGrow(DFab *pFab, size_t minSize) {
	char *buf;
	size_t size, used;

	// Initial allocation?
	if(pFab->buf == NULL) {
		size = (minSize < ChunkSize0) ? ChunkSize0 : (minSize < ChunkSize4) ? ChunkSize4 : ChunkSizeMax;
		goto Init;
		}

	// Nope... called from putchr().  Work buffer is full.
	else {
		// Double or quadruple old size?
		size = used = pFab->bufEnd - (buf = pFab->buf);
		if(size < ChunkSize4)
			size *= 2;
		else if(size < ChunkSizeMax)
			size *= 4;

		// No, already at max.  Save current buffer and allocate new one.
		else {
			if(fabSave(buf, size, pFab) != 0)
				return -1;
Init:
			buf = NULL;
			used = 0;
			}
		}

	// Get more space and reset pointers.
	if((pFab->buf = (char *) realloc((void *) buf, size)) == NULL) {
		cxlExcep.flags |= ExcepMem;
		return emsgsys(-1);
		}
	pFab->bufEnd = pFab->buf + size;
	if((pFab->flags & FabModeMask) != FabPrepend)
		pFab->bufCur = pFab->buf + used;
	else {
		pFab->bufCur = pFab->bufEnd - used;
		if(buf != NULL)

			// Extended old buffer in "prepend" mode... right-shift contents.
			fabCopy(pFab, pFab->buf, used);
		}

	return 0;
	}

// Check for fab-prepending errors and return status code.  pDatum may be NULL.
static int flagsCheck(const Datum *pDatum, DFab *pFab, ushort cflags) {

	if((pFab->flags & FabModeMask) == FabPrepend) {
		if(cflags)
			return emsg(-1, "Conversion flags not allowed when fab-prepending");
		if(pDatum != NULL && !(pDatum->type & (DStrMask | DMemMask)))
			return emsg(-1, "Cannot fab-prepend non-string datum");
		}
	return 0;
	}

// Low level routine to put a character to a fabrication object.  Return status code.
int bputc(short c, DFab *pFab) {

	if((pFab->flags & FabModeMask) == FabPrepend) {

		// Save current chunk if no space left.
		if(pFab->bufCur == pFab->buf && fabGrow(pFab, 0) != 0)
			return -1;

		// Save the character.
		*--pFab->bufCur = c;
		}
	else {
		// Save current chunk if no space left.
		if(pFab->bufCur == pFab->bufEnd && fabGrow(pFab, 0) != 0)
			return -1;

		// Save the character.
		*pFab->bufCur++ = c;
		}

	return 0;
	}

// Low level routine to put a null-terminated string to a fabrication object.  Return status code.
int bputs(const char *str, DFab *pFab) {

	if((pFab->flags & FabModeMask) == FabPrepend) {
		char *str1 = strchr(str, '\0');
		while(str1 > str)
			if(bputc(*--str1, pFab) != 0)
				return -1;
		}
	else {
		while(*str != '\0')
			if(bputc(*str++, pFab) != 0)
				return -1;
		}
	return 0;
	}

// Low level routine to put a substring to a fabrication object.  Return status code.
static int bputss(const char *strBegin, const char *strEnd, DFab *pFab) {

	if((pFab->flags & FabModeMask) == FabPrepend) {
		while(strEnd > strBegin)
			if(bputc(*--strEnd, pFab) != 0)
				return -1;
		}
	else {
		while(strBegin < strEnd)
			if(bputc(*strBegin++, pFab) != 0)
				return -1;
		}
	return 0;
	}

// "Unput" a character from a fabrication object and return it, or set an error and return -1.  Guaranteed to always work once,
// if at least one byte was previously put.
int dunputc(DFab *pFab) {

	// Any bytes in work buffer?
	if((pFab->flags & FabModeMask) == FabPrepend) {
		if(pFab->bufCur < pFab->bufEnd)
			return *pFab->bufCur++;
		}
	else if(pFab->bufCur > pFab->buf)
		return *--pFab->bufCur;

	// Empty buffer.  Return error.
	return emsg(-1, "No characters left to \"unput\"");
	}

// "Quote put" a substring or byte string to a fabrication object per cflags and return status code.  If any flags are specified
// and prepending, return an error (not supported).  If DCvtQuote1, DCvtQuote2, or DCvtQuote flag is specified, single (if
// DCvtQuote1 flag) or double (otherwise) quote characters (' or ") are added to beginning and end of converted string, with
// DCvtQuote1 flag taking precedence.  If DCvtEscChar flag is specified, backslashes, double quote characters ("), and
// non-printable characters are escaped; otherwise, if DCvtVizChar or any Viz* flag is specified, all characters are copied in
// visible form via calls to vizc() function; otherwise, string is copied literatim (raw).
static int qput(const char *str, const char *strEnd, DFab *pFab, ushort cflags) {
        short c, qChar;
	bool isChar;
	char *str1;
	char workBuf[8];

	if(flagsCheck(NULL, pFab, cflags) != 0)
		return -1;

	qChar = cflags & DCvtQuote1 ? '\'' : cflags & DCvtQuote2 ? '"' :
	 cflags & DCvtQuote ? (cflags & (DCvtVizChar | VizMask) ? '\'' : '"') : 0;

	// Handle non-escaped cases.
	if(!(cflags & DCvtEscChar)) {

		// Put string with optional "viz" conversion and/or quote-wrap.
		if(qChar && bputc(qChar, pFab) != 0)
			return -1;
		if(!(cflags & (DCvtVizChar | VizMask))) {
			if(bputss(str, strEnd, pFab) != 0)
				return -1;
			}
		else {
			while(str < strEnd)
				if(bputs(vizc(*str++, cflags), pFab) != 0)
					return -1;
			}
		}
	else {
		// Escaped-char case.  Copy and escape string.
		if((c = qChar))
			goto LitChar;

		while(str < strEnd) {
			isChar = false;
			switch(c = *str++) {
				case '"':				// Double quote
					str1 = "\\\""; break;
				case '\\':				// Backslash
					str1 = "\\\\"; break;
				case '\r':				// CR
					str1 = "\\r"; break;
				case '\n':				// NL
					str1 = "\\n"; break;
				case '\t':				// Tab
					str1 = "\\t"; break;
				case '\b':				// Backspace
					str1 = "\\b"; break;
				case '\f':				// Form feed
					str1 = "\\f"; break;
				case '\v':				// Vertical tab
					str1 = "\\v"; break;
				case 033:				// Escape
					str1 = "\\e"; break;
				default:
					if(c < ' ' || c >= 0x7F)	// Non-printable character.
						sprintf(str1 = workBuf, "\\%.3ho", c);
					else				// Literal character.
LitChar:
						isChar = true;
				}
			if((isChar ? bputc(c, pFab) : bputs(str1, pFab)) != 0)
				return -1;
			}
		}

	// Store trailing quote character if applicable, and return status.
	return qChar ? bputc(qChar, pFab) : 0;
	}

// Put a character to a fabrication object per cflags.  Return status code.
int dputc(short c, DFab *pFab, ushort cflags) {

	if(flagsCheck(NULL, pFab, cflags) != 0)
		return -1;

	// Do vizc() conversion here if no other conversion needed.
	if((cflags & (DCvtVizChar | VizMask)) && !(cflags & ~(DCvtVizChar | VizMask)))
		return bputs(vizc(c, cflags), pFab);

	// Not vizc() only.  Call qput() for all other conversions (including a null character), or output raw character.
	if(cflags & (DCvtVizChar | VizMask | DCvtEscChar | DCvtQuoteMask)) {
		char workBuf[1];

		workBuf[0] = c;
		return qput(workBuf, workBuf + 1, pFab, (cflags & DCvtQuoteMask) == DCvtQuote ? cflags | DCvtQuote1 : cflags);
		}

	return bputc(c, pFab);
	}

// Put a string to a fabrication object per cflags.  Return status code.
int dputs(const char *str, DFab *pFab, ushort cflags) {

	return qput(str, strchr(str, '\0'), pFab, cflags);
	}

// Put a substring (up to "len" characters) to a fabrication object per cflags.  Return status code.
int dputsubstr(const char *str, size_t len, DFab *pFab, ushort cflags) {
	const char *str1 = strchr(str, '\0');
	const char *str2 = str + len;

	return qput(str, str1 < str2 ? str1 : str2, pFab, cflags);
	}

// Put "size" bytes in memory to a fabrication object per cflags, and return status code.
int dputmem(const void *memPtr, size_t size, DFab *pFab, ushort cflags) {

	return qput(memPtr, memPtr + size, pFab, cflags);
	}

// Convert datum in *pSrc to a human-readable string if conditions are met.  If primitive type or character or string not
// requiring conversion found, save pointer to result in *pDest and return 0; if character or string found and DCvtEscChar,
// DCvtQuote1, DCvtQuote2, DCvtQuote, DCvtVizChar, or Viz* flag set (requiring a conversion), return 1 or 2, respectively; if
// array found, return 3; otherwise (byte string found), return 4.
static int dtos1(char **pDest, const Datum *pSrc, ushort cflags) {
	static char workBuf[64];
	char *str = workBuf;

	// Check datum type.
	switch(pSrc->type) {
		case dat_nil:
			str = cflags & DCvtShowNil ? "nil" : "";
			break;
		case dat_false:
			str = "false";
			break;
		case dat_true:
			str = "true";
			break;
		case dat_char:		// "Conversion" required if null byte (can't be converted to string).
			if((cflags & (DCvtQuoteMask | DCvtEscChar | DCvtVizChar | VizMask)) || pSrc->u.c == '\0')
				return 1;
			workBuf[0] = pSrc->u.c;
			workBuf[1] = '\0';
			break;
		case dat_int:
			if(cflags & DCvtThouSep)
				str = intf(pSrc->u.intNum);
			else
				sprintf(workBuf, "%ld", pSrc->u.intNum);
			break;
		case dat_uint:
			if(cflags & DCvtThouSep)
				str = uintf(pSrc->u.uintNum);
			else
				sprintf(workBuf, "%lu", pSrc->u.uintNum);
			break;
		case dat_real:
			sprintf(workBuf, "%.16lg", pSrc->u.realNum);
			break;
		case dat_miniStr:
		case dat_longStr:
		case dat_longStrRef:
			if(cflags & (DCvtQuoteMask | DCvtEscChar | DCvtVizChar | VizMask))
				return 2;
			str = pSrc->str;
			break;
		case dat_array:
		case dat_arrayRef:
			return 3;
		default:	/* Byte string */
			return 4;
		}
	*pDest = str;
	return 0;
	}

// Write a Datum object to a fabrication object in string form, per cflags, and return status code.
int dputd(const Datum *pDatum, DFab *pFab, const char *delim, ushort cflags) {
	char *str;

	if(flagsCheck(pDatum, pFab, cflags) != 0)
		return -1;
	switch(dtos1(&str, pDatum, cflags)) {
		case 0:		// Keyword or simple string.
			return bputs(str, pFab);
		case 1:		// Character conversion.
			return dputc(pDatum->u.c, pFab, cflags);
		case 2:		// String conversion.
			return dputs(pDatum->str, pFab, cflags);
		case 3:		// Array.
			return aput(pDatum->u.pArray, pFab, delim, cflags);
		}

	// Byte string.
	return qput(pDatum->u.mem.ptr, pDatum->u.mem.ptr + pDatum->u.mem.size, pFab, cflags);
	}

// Put formatted text to a fabrication object.  Return status code.
int dputf(DFab *pFab, ushort cflags, const char *fmt, ...) {
	char *str;
	va_list varArgList;

	va_start(varArgList, fmt);
	(void) vasprintf(&str, fmt, varArgList);
	va_end(varArgList);
	if(str == NULL)
		return emsgsys(-1);

	int rtnCode = dputs(str, pFab, cflags);
	free((void *) str);
	return rtnCode;
	}

// Prepare for writing to a fabrication object, given pointer to object to initialize, (possibly NULL) pointer to destination
// Datum object, and fflags.  If pDatum is NULL, create a new Datum object (which can be retrieved by the caller from the DFab
// record).  If the FabTrack flag is set, create a tracked object, otherwise untracked.  If pDatum is not NULL and the
// FabPrepend or FabAppend flag is set, keep existing string value in *pDatum (and prepend or append to it); otherwise, clear
// it.  Return status code.
static int dprep(DFab *pFab, Datum *pDatum, ushort fflags) {

	pFab->stack = NULL;
	pFab->buf = NULL;

	// Initialize Datum object.
	if(pDatum == NULL) {
		if(dmake(&pDatum, fflags & FabTrack) != 0)
			return -1;
		goto SetNull;
		}
	else if((fflags & FabModeMask) == FabClear || !dtypstr(pDatum)) {
		if(pDatum->type != dat_nil)
			fflags = (fflags & ~FabModeMask) | FabClear;	// Not string or nil: force "clear" mode.
SetNull:
		dsetnull(pDatum);
		}

	pFab->pDatum = pDatum;
	pFab->flags = fflags;

	// Initialize fabrication buffers, preserving existing string value in Datum object if appending or prepending.
	if((fflags & FabModeMask) != FabClear) {
		size_t used = strlen(pDatum->str);
		if(used < ChunkSizeMax) {
			if(fabGrow(pFab, used) != 0)			// Allocate a work buffer.
				return -1;
			if(used > 0) {					// If existing string not null...
				fabCopy(pFab, pDatum->str, used);	// copy it to work buffer, left or right justified...
				dsetnull(pDatum);			// and set Datum object to null.
				}
			return 0;
			}

		// Appending or prepending to existing string >= ChunkSizeMax in length.  Save it in a DChunk object and
		// re-initialize Datum object (without calling free()).
		if(fabSave(pDatum->str, used, pFab) != 0)
			return -1;
		dinit(pDatum);
		dsetnull(pDatum);
		}

	return fabGrow(pFab, 0);
	}

// Open a fabrication object via dprep().  An untracked Datum object will be created.  Return status code.
int dopen(DFab *pFab) {

	return dprep(pFab, NULL, FabClear);
	}

// Open a fabrication object via dprep().  A tracked Datum object will be created.  Return status code.
int dopentrack(DFab *pFab) {

	return dprep(pFab, NULL, FabTrack | FabClear);
	}

// Open a fabrication object via dprep() with given Datum object (or NULL) and append flag.  Return status code.
int dopenwith(DFab *pFab, Datum *pDatum, ushort mode) {

	return (pDatum == NULL) ? emsg(-1, "Datum pointer cannot be NULL") : dprep(pFab, pDatum, mode);
	}

// Return true if a fabrication object is empty, otherwise false.
bool dfabempty(const DFab *pFab) {

	return pFab->bufCur == (((pFab->flags & FabModeMask) == FabPrepend) ? pFab->bufEnd : pFab->buf) &&
	 pFab->stack == NULL;
	}

// Convert a non-reference string datum is a string reference type.  Convert a mini-string datum to an allocated one, if
// necessary.  Return status code.
static int forceStrRef(Datum *pDatum) {

	if(pDatum->type == dat_miniStr) {
		char *str;

		if(salloc(&str, strlen(pDatum->str) + 1, NULL) != 0)
			return -1;
		strcpy(str, pDatum->str);
		dsetstrref(str, pDatum);
		}
	else
		pDatum->type = dat_longStrRef;

	return 0;
	}

// End a fabrication object write operation and convert target Datum object to a string or byte string, according to 'type'.
// Return status code.
int dclose(DFab *pFab, ushort type) {

	// Validate type.  Ensure that only one bit (or none) is set.
	if(type != 0) {
		int bitCount, i;
		uint t;

		if(type & ~FabCloseMask)
			goto TypeErr;
		bitCount = i = 0;
		t = type;
		do {
			bitCount += (t & 1);
			t >>= 1;
			} while(++i < FabCloseBits);
		if(bitCount > 1)
TypeErr:
			return emsgf(-1, "Unknown dclose() type, %hu", type);
		}

	// If still at starting point (no bytes written), change Datum object to empty byte string or string reference if force;
	// otherwise, leave as a null (mini) string.
	if(dfabempty(pFab)) {
		if(type & (FabMem | FabMemRef)) {
			dsetmemref(NULL, 0, pFab->pDatum);
			if(type == FabMem)
				pFab->pDatum->type = dat_byteStr;
			}
		else if(type == FabStrRef && forceStrRef(pFab->pDatum) != 0)
			return -1;
		free((void *) pFab->buf);
		}

	// At least one byte was saved.  If still on first chunk, check if it contains any imbedded null bytes and save as
	// correct type or return error as appropriate.
	else {
		char *str;
		size_t len;

		if((pFab->flags & FabModeMask) == FabPrepend) {
			str = pFab->bufCur;
			len = pFab->bufEnd - str;
			}
		else {
			str = pFab->buf;
			len = pFab->bufCur - str;
			}

		if(pFab->stack == NULL) {

			// First chunk (and no more).  Whole string is in buffer.
			bool isBinary = (memchr(str, '\0', len) != NULL);

			if(isBinary && type & (FabStr | FabStrRef))
				goto BinErr;
			if(((isBinary || type & (FabMem | FabMemRef)) ? dsetmem((void *) str, len, pFab->pDatum) :
			 dsetsubstr(str, len, pFab->pDatum)) != 0)
				return -1;
			free((void *) pFab->buf);
			if(type == FabMemRef)
				pFab->pDatum->type = dat_byteStrRef;
			else if(type == FabStrRef && forceStrRef(pFab->pDatum) != 0)
				return -1;
			}
		else {
			// Not first chunk.  Add last one (which can't be empty) to stack and convert to a long string or byte
			// string.  Remember pointer to work buffer in buf0 if prepending and work buffer is not full so that
			// pointer can be passed to free() later instead of 'str'.
			char *buf0 = (str != pFab->buf) ? pFab->buf : NULL;
			if(fabSave(str, len, pFab) != 0)
				return -1;
			else {
				DChunk *pChunk;
				size_t size;
				char *str0;
				bool isBinary = false;
				Datum *pDatum = pFab->pDatum;
				char workBuf[sizeof(DMem)];

				// Get total length.
				len = 0;
				pChunk = pFab->stack;
				do {
					len += pChunk->mem.size;
					} while((pChunk = pChunk->next) != NULL);

				// Get space for concatenated chunks.
				if(salloc(&str, len + 1, workBuf) != 0)
					return -1;
				str0 = str;

				// Copy string pieces into it and check for any imbedded null bytes.
				pChunk = pFab->stack;
				if((pFab->flags & FabModeMask) == FabPrepend) {

					// Prepending: copy chunks from beginning to end.
					do {
						size = pChunk->mem.size;
						if(!isBinary && memchr(pChunk->mem.ptr, '\0', size) != NULL)
							isBinary = true;
						str = memstpcpy((void *) str, (void *) pChunk->mem.ptr, size);
						if(buf0 != NULL) {
							free((void *) buf0);
							buf0 = NULL;
							}
						else
							free((void *) pChunk->mem.ptr);
						} while((pChunk = pChunk->next) != NULL);
					*str = '\0';
					}
				else {
					// Not prepending: copy chunks from end to beginning.
					str += len + 1;
					*--str = '\0';
					do {
						size = pChunk->mem.size;
						if(!isBinary && memchr(pChunk->mem.ptr, '\0', size) != NULL)
							isBinary = true;
						str -= size;
						memcpy((void *) str, (void *) pChunk->mem.ptr, size);
						free((void *) pChunk->mem.ptr);
						} while((pChunk = pChunk->next) != NULL);
					}

				// Set byte string in Datum object if able.
				if(isBinary && type & (FabStr | FabStrRef))
BinErr:
					return emsg(-1, "Cannot convert binary data to string");
				if(isBinary || type & (FabMem | FabMemRef)) {
					if(str0 == workBuf) {			// "Mini" byte string in workBuf.
						if(dsetmem((void *) workBuf, len, pDatum) != 0)
							return -1;
						if(type == FabMemRef)
							pDatum->type = dat_byteStrRef;
						}
					else {					// Byte string in memory.
						dsetmemref((void *) str0, len, pDatum);
						if(type != FabMemRef)
							pDatum->type = dat_byteStr;
						}
					}
				else if(str0 == workBuf) {			// Mini-string in workBuf.
					if(type == FabStrRef) {
						if(salloc(&str, len + 1, NULL) != 0)
							return -1;
						dsetstrref(str, pDatum);
						}
					else {
						dsetnull(pDatum);
						str = pDatum->u.miniStr;
						}
					strcpy(str, workBuf);
					}
				else if(type == FabStrRef)			// Long string in memory.
					dsetstrref(str0, pDatum);
				else
					dadoptstr(str0, pDatum);
				}
			}
		}

	return 0;
	}

// Enable tracking on a Datum object by adding it to the garbage collection stack (if not already present).
void dtrack(Datum *pDatum) {

	for(Datum *pDatum1 = datGarbHead; pDatum1 != NULL; pDatum1 = pDatum1->next)
		if(pDatum1 == pDatum)
			return;
	pDatum->next = datGarbHead;
	datGarbHead = pDatum;
	}

// Stop tracking a Datum object by removing it from the garbage collection stack (if present).
void duntrack(Datum *pDatum) {

	if(pDatum == datGarbHead) {
		datGarbHead = datGarbHead->next;
		pDatum->next = NULL;
		}
	else
		for(Datum *pDatum1 = datGarbHead; pDatum1 != NULL; pDatum1 = pDatum1->next)
			if(pDatum1->next == pDatum) {
				pDatum1->next = pDatum->next;
				pDatum->next = NULL;
				break;
				}
	}

// Copy one datum to another.  Return status code.
int dcpy(Datum *pDest, const Datum *pSrc) {

	switch(pSrc->type) {
		case dat_nil:
		case dat_false:
		case dat_true:
			dclear(pDest);
			pDest->type = pSrc->type;
			pDest->str = NULL;
			break;
		case dat_char:
			dsetchr(pSrc->u.c, pDest);
			break;
		case dat_int:
			dsetint(pSrc->u.intNum, pDest);
			break;
		case dat_uint:
			dsetuint(pSrc->u.uintNum, pDest);
			break;
		case dat_real:
			dsetreal(pSrc->u.realNum, pDest);
			break;
		case dat_miniStr:
		case dat_longStr:
		case dat_longStrRef:
			return dsetstr(pSrc->str, pDest);
		case dat_byteStr:
			return dsetmem(pSrc->u.mem.ptr, pSrc->u.mem.size, pDest);
		case dat_byteStrRef:
			dsetmemref(pSrc->u.mem.ptr, pSrc->u.mem.size, pDest);
			break;
		case dat_array:
			return dsetarray(pSrc->u.pArray, pDest);
		default:	// dat_arrayRef
			dsetarrayref(pSrc->u.pArray, pDest);
		}
	return 0;
	}

// Compare one Datum to another per dflags and return true if values are equal, otherwise false.  If DOpIgnore flag is set,
// ignore case in character and string comparisons.
bool deq(const Datum *pDatum1, const Datum *pDatum2, ushort dflags) {

	switch(pDatum1->type) {
		case dat_nil:
		case dat_false:
		case dat_true:
			return pDatum2->type == pDatum1->type;
		case dat_char:
			return pDatum2->type == dat_char && (pDatum2->u.c == pDatum1->u.c ||
			 ((dflags & DOpIgnore) && tolower(pDatum2->u.c) == tolower(pDatum1->u.c)));
		case dat_int:
			return (pDatum2->type == dat_int && pDatum2->u.intNum == pDatum1->u.intNum) ||
			 (pDatum2->type == dat_uint &&
			  pDatum1->u.intNum >= 0 && pDatum2->u.uintNum == (ulong) pDatum1->u.intNum) ||
			 (pDatum2->type == dat_real && pDatum2->u.realNum == (double) pDatum1->u.intNum);
		case dat_uint:
			return (pDatum2->type == dat_uint && pDatum2->u.uintNum == pDatum1->u.uintNum) ||
			 (pDatum2->type == dat_int
			  && pDatum2->u.intNum >= 0 && (ulong) pDatum2->u.intNum == pDatum1->u.uintNum) ||
			 (pDatum2->type == dat_real && pDatum2->u.realNum == (double) pDatum1->u.uintNum);
		case dat_real:
			return (pDatum2->type == dat_real && pDatum2->u.realNum == pDatum1->u.realNum) ||
			 (pDatum2->type == dat_int && (double) pDatum2->u.intNum == pDatum1->u.realNum) ||
			 (pDatum2->type == dat_uint && (double) pDatum2->u.uintNum == pDatum1->u.realNum);
		case dat_miniStr:
		case dat_longStr:
		case dat_longStrRef:
			if(dtypstr(pDatum2)) {
				int (*cmp)(const char *str1, const char *str2) = dflags & DOpIgnore ? strcasecmp : strcmp;
				return cmp(pDatum1->str, pDatum2->str) == 0;
				}
			return false;
		case dat_byteStr:
		case dat_byteStrRef:
			return dtypmem(pDatum2) && pDatum1->u.mem.size == pDatum2->u.mem.size &&
			 memcmp(pDatum1->u.mem.ptr, pDatum2->u.mem.ptr, pDatum1->u.mem.size) == 0;
		}

	// dat_array, dat_arrayRef
	return dtyparray(pDatum2) ? aeq(pDatum1->u.pArray, pDatum2->u.pArray, dflags & DOpIgnore) : false;
	}

// Copy string from 'str' to 'pDatum', adding a single quote (') at beginning and end and converting single quote characters
// to '\''.  Return status.
int dshquote(const char *str, Datum *pDatum) {
	ptrdiff_t len;
	const char *str0, *str1;
	bool apostrophe;
	DFab dest;

	str0 = str;
	if(dopenwith(&dest, pDatum, FabClear) != 0)
		return -1;
	for(;;) {
		if(*str == '\0')				// If null...
			break;					// we're done.
		if(*str == '\'') {				// If ' ...
			apostrophe = true;			// convert it.
			len = 0;
			++str;
			}
		else {
			apostrophe = false;
			if((str1 = strchr(str, '\'')) == NULL)	// Search for next ' ...
				str1 = strchr(str, '\0');	// or null if not found.
			len = str1 - str;			// Length of next chunk to copy.
			str1 = str;
			str += len;
			}

		if(apostrophe) {				// Copy conversion literal.
			if(bputs("\\'", &dest) != 0)
				return -1;
			}					// Copy chunk.
		else if(bputc('\'', &dest) != 0 || bputss(str1, str1 + len, &dest) != 0 || bputc('\'', &dest) != 0)
			return -1;
		}
	if(*str0 == '\0' && bputs("''", &dest) != 0)		// Wrap it up.
		return -1;
	return dclose(&dest, FabStr);
	}

// Convert datum in *pSrc to string per cflags and save in *pDest.  Store "delim" delimiter (which may be NULL) between elements
// if datum is an array.  Return zero if successful, -1 if error, or 1 if successful but endless array recursion was detected.
int dtos(Datum *pDest, const Datum *pSrc, const char *delim, ushort cflags) {
	char *str;
	DFab fab;
	int rtnCode = 0;

	// Do simple conversion if possible.
	switch(dtos1(&str, pSrc, cflags)) {
		case -1:	// Error.
			return -1;
		case 0:		// Keyword or simple string.
			return dsetstr(str, pDest);
		}

	// Have character, string, byte string, or array needing conversion.  Open fabrication object and call appropriate
	// routine.
	return dopenwith(&fab, pDest, FabClear) != 0 ||
	 (dischr(pSrc) ? dputc(pSrc->u.c, &fab, cflags) :
	 dtypstr(pSrc) ? dputs(pSrc->str, &fab, cflags) :
	 dtypmem(pSrc) ? qput(pSrc->u.mem.ptr, pSrc->u.mem.ptr + pSrc->u.mem.size, &fab, cflags) :
	 (rtnCode = aput(pSrc->u.pArray, &fab, delim, cflags))) < 0 ||
	 dclose(&fab, FabStr) != 0 ? -1 : rtnCode;
	}
