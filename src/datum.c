// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// datum.c		Datum object routines.

#include "stdos.h"
#include "cxl/excep.h"
#include "cxl/datum.h"
#include "cxl/string.h"
#if DDebug
#include "cxl/ioext.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

// Local definitions.
#if DFabTest
#define ChunkSize0	32			// Starting size of fabrication chunks/pieces.  *TEST VALUE*
#define ChunkSize4	256
#define ChunkSizeMax	16384
#else
#define ChunkSize0	64			// Starting size of fabrication chunks/pieces.
#define ChunkSize4	512			// Size at which to begin quadrupling until hit maximum.
#define ChunkSizeMax	32768			// Maximum size (32K).
#endif

// Global variables.
Datum *datGarbHead = NULL;			// Head of list of temporary Datum objects, for "garbage collection".

#if DDebug
extern FILE *logfile;

// Dump Datum object to log file.
void ddump(const Datum *pDatum, char *tag) {
	char *label;
	char *membName;

	fprintf(logfile, "%s\n\taddr: %.8X\n", tag, (uint) pDatum);
	fflush(logfile);
	if(pDatum != NULL)
		switch(pDatum->type) {
			case dat_nil:
				label = "nil";
				goto Konst;
			case dat_false:
				label = "false";
				goto Konst;
			case dat_true:
				label = "true";
Konst:
				fprintf(logfile, "\t%s\n", label);
				break;
			case dat_int:
				fprintf(logfile, "\tint: %ld\n", pDatum->u.intNum);
				break;
			case dat_uint:
				fprintf(logfile, "\tuint: %lu\n", pDatum->u.uintNum);
				break;
			case dat_real:
				fprintf(logfile, "\treal: %.2lf\n", pDatum->u.realNum);
				break;
			case dat_miniStr:
				label = "MINI STR";
				goto String;
			case dat_soloStr:
			case dat_soloStrRef:
				label = (pDatum->type == dat_soloStr) ? "HEAP STR" : "REF STR";
				if(pDatum->u.soloStr == NULL) {
					membName = "u.soloStr";
					goto Bad;
					}
String:
				if(pDatum->str == NULL) {
					membName = "str";
Bad:
					fprintf(logfile,
					 "*** %s member of Datum object passed to ddump() is NULL ***\n", membName);
					}
				else {
					short c;
					char *str = pDatum->str;
					fprintf(logfile, "\t%s (%hu): \"", label, pDatum->type);
					while((c = *str++) != '\0')
						fputs(vizc(c, VizBaseDef), logfile);
					fputs("\"\n", logfile);
					}
				break;
			case dat_blob:
			case dat_blobRef:
				fprintf(logfile, "\t%s (size %lu): %.8X\n", pDatum->type == dat_blobRef ? "BLOBREF" : "BLOB",
				 pDatum->u.blob.size, (uint) pDatum->u.blob.mem);
				break;
			}
	}
#endif

// Initialize a Datum object to nil.  It is assumed any heap storage has already been freed.
#if DCaller
void dinit(Datum *pDatum, const char *caller) {
#else
void dinit(Datum *pDatum) {
#endif
	pDatum->type = dat_nil;
	pDatum->str = NULL;
#if DCaller
	fprintf(logfile, "dinit(): initialized object %.8X for %s()\n", (uint) pDatum, caller);
#endif
	}

// Clear a Datum object and set it to nil.
void dclear(Datum *pDatum) {

	// Free any string or blob storage.
	switch(pDatum->type) {
		case dat_soloStr:
#if DDebug
			fprintf(logfile, "dclear(): free [%.8X] SOLO ", (uint) pDatum->str);
			ddump(pDatum, "from...");
			free((void *) pDatum->str);
			fputs("dclear(): DONE.\n", logfile);
			fflush(logfile);
#else
			free((void *) pDatum->str);
#endif
			break;
		case dat_blob:
#if DDebug
			fprintf(logfile, "dclear(): free [%.8X] BLOB, size %lu", (uint) pDatum->u.blob.mem,
			 pDatum->u.blob.size);
			ddump(pDatum, "from...");
			if(pDatum->u.blob.mem != NULL)
				free((void *) pDatum->u.blob.mem);
			fputs("dclear(): DONE.\n", logfile);
			fflush(logfile);
#else
			if(pDatum->u.blob.mem != NULL)
				free((void *) pDatum->u.blob.mem);
#endif
		}
#if DCaller
	dinit(pDatum, "dclear");
#else
	dinit(pDatum);
#endif
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
	pDatum->str = NULL;
	}

// Set a blob value in a Datum object (copy to heap), given void pointer to object and size in bytes, which may be zero.  Return
// status code.
int dsetblob(const void *mem, size_t size, Datum *pDatum) {
	DBlob *pBlob = &pDatum->u.blob;

	dclear(pDatum);
	pDatum->type = dat_blob;
	pDatum->str = NULL;
	if((pBlob->size = size) == 0)
		pBlob->mem = NULL;
	else {
		if((pBlob->mem = malloc(size)) == NULL) {
			cxlExcep.flags |= ExcepMem;
			return emsgsys(-1);
			}
		memcpy(pBlob->mem, mem, size);
		}
	return 0;
	}

// Set a blob reference in a Datum object, given void pointer to object and size in bytes.
void dsetblobref(void *mem, size_t size, Datum *pDatum) {
	DBlob *pBlob = &pDatum->u.blob;

	dclear(pDatum);
	pDatum->type = dat_blobRef;
	pDatum->str = NULL;
	pBlob->mem = mem;
	pBlob->size = size;
	}

// Set a single-character (string) value in a Datum object.
void dsetchr(short c, Datum *pDatum) {

	dclear(pDatum);
	(pDatum->str = pDatum->u.miniStr)[0] = c;
	pDatum->u.miniStr[1] = '\0';
	pDatum->type = dat_miniStr;
	}

// Set a signed integer value in a Datum object.
void dsetint(long i, Datum *pDatum) {

	dclear(pDatum);
	pDatum->u.intNum = i;
	pDatum->type = dat_int;
	pDatum->str = NULL;
	}

// Set an unsigned integer value in a Datum object.
void dsetuint(ulong u, Datum *pDatum) {

	dclear(pDatum);
	pDatum->u.uintNum = u;
	pDatum->type = dat_uint;
	pDatum->str = NULL;
	}

// Set a real number value in a Datum object.
void dsetreal(double d, Datum *pDatum) {

	dclear(pDatum);
	pDatum->u.realNum = d;
	pDatum->type = dat_real;
	pDatum->str = NULL;
	}

// Get space for a string and set *pStr to it.  If a mini string will work, use caller's mini buffer 'miniBuf' (if not NULL).
// Return status code.
static int salloc(char **pStr, size_t len, char *miniBuf) {

	if(miniBuf != NULL && len <= sizeof(DBlob))
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
	if(len > sizeof(DBlob)) {
#if DDebug
		fprintf(logfile, "dsalloc(): getting heap string for %.8X for len %lu...\n", (uint) pDatum, len);
#endif
		if(salloc(&pDatum->u.soloStr, len, NULL) != 0)
			return -1;
		*(pDatum->str = pDatum->u.soloStr) = '\0';
		pDatum->type = dat_soloStr;
		}
#if DDebug
	else
		fprintf(logfile, "dsalloc(): retaining MINI string (in pDatum %.8X) for len %lu\n", (uint) pDatum, len);
#endif
	return 0;
	}

// Set a string reference in a Datum object.
static void setStrRef(char *str, Datum *pDatum, DatumType type) {

	dclear(pDatum);
	pDatum->str = pDatum->u.soloStr = str;
	pDatum->type = type;
	}

// Set a string value currently on the heap in a Datum object; that is, "adopt" the string and free it when the Datum object is
// cleared.
void dsetheapstr(char *str, Datum *pDatum) {

	setStrRef(str, pDatum, dat_soloStr);
	}

// Set a string reference in a Datum object.
void dsetstrref(char *str, Datum *pDatum) {

	setStrRef(str, pDatum, dat_soloStrRef);
	}

// Set a substring (which is assumed to not contain any embedded null bytes) in a Datum object, given pointer and length.
// Routine also assumes that the source string may be all or part of the dest object.  Return status code.
int dsetsubstr(const char *str, size_t len, Datum *pDatum) {
	char *str0;
	char workBuf[sizeof(DBlob)];

	if(salloc(&str0, len + 1, workBuf) != 0)	// Get space for string...
		return -1;
	stplcpy(str0, str, len + 1);			// and copy string to new buffer.
	if(str0 == workBuf) {				// If mini string...
		dsetnull(pDatum);			// clear pDatum...
		strcpy(pDatum->u.miniStr, workBuf);// and copy it.
		}
	else
		dsetheapstr(str0, pDatum);		// Otherwise, set heap string in pDatum.
	return 0;
	}

// Set a string value in a Datum object.  Return status code.
int dsetstr(const char *str, Datum *pDatum) {

	return dsetsubstr(str, strlen(str), pDatum);
	}

// Transfer contents of one Datum object to another.  Return pDest.
Datum *datxfer(Datum *pDest, Datum *pSrc) {
	Datum *pDatum = pDest->next;			// Save the 'next' pointer...
#if DDebug
	static char xMsg[64];
	sprintf(xMsg, "datxfer(): src [%.8X] BEFORE...", (uint) pSrc);
	ddump(pSrc, xMsg);
	sprintf(xMsg, "datxfer(): dest [%.8X] BEFORE...", (uint) pDest);
	ddump(pDest, xMsg);
#endif
	dclear(pDest);					// free dest...
	*pDest = *pSrc;					// copy the whole burrito...
	pDest->next = pDatum;				// restore 'next' pointer...
	if(pDest->type == dat_miniStr)			// fix mini-string pointer...
		pDest->str = pDest->u.miniStr;
#if DCaller
	dinit(pSrc, "datxfer");
#else
	dinit(pSrc);					// initialize (clear) the source...
#endif
#if DDebug
	ddump(pSrc, "datxfer(): src AFTER...");
	ddump(pDest, "datxfer(): dest AFTER...");
#endif
	return pDest;					// and return result.
	}

// Return true if a Datum object is nil; otherwise, false.
bool disnil(const Datum *pDatum) {

	return pDatum->type == dat_nil;
	}

// Return true if a Datum object is a false Boolean value; otherwise, false.
bool disfalse(const Datum *pDatum) {

	return pDatum->type == dat_false;
	}

// Return true if a Datum object is a true Boolean value; otherwise, false.
bool distrue(const Datum *pDatum) {

	return pDatum->type == dat_true;
	}

// Return true if a Datum object is a null string; otherwise, false.
bool disnull(const Datum *pDatum) {

	return (pDatum->type & DStrMask) && *pDatum->str == '\0';
	}

// Create Datum object and set *ppDatum to it.  If 'track' is true, add it to the garbage collection stack.  Return status code.
#if DCaller
static int dmake(Datum **ppDatum, bool track, const char *caller, const char *dName) {
#else
static int dmake(Datum **ppDatum, bool track) {
#endif
	Datum *pDatum;

#if DDebug && 0
	fputs("dmake(): Dump object list...\n", logfile);
	for(pDatum = datGarbHead; pDatum != NULL; pDatum = pDatum->next)
		ddump(pDatum, "dmake() DUMP");
#endif
	// Get new object...
	if((pDatum = (Datum *) malloc(sizeof(Datum))) == NULL) {
		cxlExcep.flags |= ExcepMem;
		return emsgsys(-1);
		}
#if DDebug
#if DCaller
fprintf(logfile, "dmake(): Allocated %.8X, type %s for %s(), var '%s'\n", (uint) pDatum, track ? "TRACK" : "Perm", caller,
 dName);
#else
fprintf(logfile, "dmake(): Allocated %.8X, type %s\n", (uint) pDatum, track ? "TRACK" : "Perm");
#endif
#endif
	// add it to garbage collection stack (if applicable)...
	if(track) {
		pDatum->next = datGarbHead;
		datGarbHead = pDatum;
		}
	else
		pDatum->next = NULL;

	// initialize it...
#if DCaller
	dinit(pDatum, "dmake");
#else
	dinit(pDatum);
#endif
	// and return the spoils.
	*ppDatum = pDatum;
	return 0;
	}

// Create Datum object and set *ppDatum to it.  Object is not added it to the garbage collection stack.  Return status code.
#if DCaller
int dnew(Datum **ppDatum, const char *caller, const char *dName) {

	return dmake(ppDatum, false, caller, dName);
#else
int dnew(Datum **ppDatum) {

	return dmake(ppDatum, false);
#endif
	}

// Create (tracked) Datum object, set *ppDatum to it, and add it to the garbage collection stack.  Return status code.
#if DCaller
int dnewtrack(Datum **ppDatum, const char *caller, const char *dName) {

	return dmake(ppDatum, true, caller, dName);
#else
int dnewtrack(Datum **ppDatum) {

	return dmake(ppDatum, true);
#endif
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
	pChunk->blob.mem = (void *) str;
	pChunk->blob.size = len;
#if DDebug
	fputs("fabSave(", logfile);
	fvizs((void *) str, len > 32 ? 32 : len, logfile);
	fprintf(logfile, " [%.8X],%lu,%.8X): done.\n", (uint) str, len, (uint) pFab);
#endif
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

	// Nope... called from dputc().  Work buffer is full.
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
#if DDebug
	fprintf(logfile, "fabGrow(%.8X,%lu): Calling realloc() at %.8X, size %lu, used %lu...\n", (uint) pFab, minSize,
	 (uint) buf, size, used);
#endif
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
#if DDebug
	fprintf(logfile, "fabGrow(): done.  buf %.8X, bufCur %.8X, bufEnd %.8X\n",
	 (uint) pFab->buf, (uint) pFab->bufCur, (uint) pFab->bufEnd);
#endif
	return 0;
	}

// Put a character to a fabrication object.  Return status code.
int dputc(short c, DFab *pFab) {

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
#if DDebug
	fprintf(logfile, "dputc(): Saved char %s at %.8X, buf %.8X, buflen %lu.\n", vizc(c, VizBaseDef),
	 (uint) pFab->bufCur - ((pFab->flags & FabModeMask) == FabPrepend) ? 0 : 1, (uint) pFab->buf,
	 pFab->bufEnd - pFab->buf);
#endif
	return 0;
	}

// "Unput" a character from a fabrication object.  Guaranteed to always work once, if at least one byte was previously put.
// Return status code, including error if work buffer is empty.
int dunputc(DFab *pFab) {

	// Any bytes in work buffer?
	if((pFab->flags & FabModeMask) == FabPrepend) {
		if(pFab->bufCur < pFab->bufEnd) {
			++pFab->bufCur;
			return 0;
			}
		}
	else if(pFab->bufCur > pFab->buf) {
		--pFab->bufCur;
		return 0;
		}

	// Empty buffer.  Return error.
	return emsg(-1, "No bytes left to \"unput\"");
	}

// Put a string to a fabrication object.  Return status code.
int dputs(const char *str, DFab *pFab) {

	if((pFab->flags & FabModeMask) == FabPrepend) {
		char *str1 = strchr(str, '\0');
		while(str1 > str)
			if(dputc(*--str1, pFab) != 0)
				return -1;
		}
	else {
		while(*str != '\0')
			if(dputc(*str++, pFab) != 0)
				return -1;
		}
	return 0;
	}

// Put 'size' bytes to a fabrication object.  Return status code.
int dputmem(const void *mem, size_t size, DFab *pFab) {

	if((pFab->flags & FabModeMask) == FabPrepend) {
		char *str = (char *) mem + size;
		while(size-- > 0)
			if(dputc(*--str, pFab) != 0)
				return -1;
		}
	else {
		char *str = (char *) mem;
		while(size-- > 0)
			if(dputc(*str++, pFab) != 0)
				return -1;
		}
	return 0;
	}

// Put the contents of a Datum object to a fabrication object.  Return status code.
int dputd(const Datum *pDatum, DFab *pFab) {
	char *str;
	char workBuf[80];

	switch(pDatum->type) {
		case dat_nil:
			return 0;
		case dat_miniStr:
		case dat_soloStr:
		case dat_soloStrRef:
			str = pDatum->str;
			goto Put;
		case dat_int:
			sprintf(str = workBuf, "%ld", pDatum->u.intNum);
			goto Put;
		case dat_uint:
			sprintf(str = workBuf, "%lu", pDatum->u.uintNum);
			goto Put;
		case dat_real:
			sprintf(str = workBuf, "%lf", pDatum->u.realNum);
Put:
			return dputs(str, pFab);
		case dat_blob:
		case dat_blobRef:
			return dputmem(pDatum->u.blob.mem, pDatum->u.blob.size, pFab);
		}

	// Invalid pDatum type.
	return emsgf(-1, "Cannot convert Datum (type %hu) to string", pDatum->type);
	}

// Put formatted text to a fabrication object.  Return status code.
int dputf(DFab *pFab, const char *fmt, ...) {
	char *str;
	va_list varArgList;

	va_start(varArgList, fmt);
	(void) vasprintf(&str, fmt, varArgList);
	va_end(varArgList);
	if(str == NULL)
		return emsgsys(-1);

	int rtnCode = dputs(str, pFab);
	free((void *) str);
	return rtnCode;
	}

// Flags for fabrication object creation (combined with flags defined in header file).
#define FabTrack	0x0004			// Create tracked Datum object.

// Prepare for writing to a fabrication object, given pointer to object to initialize, (possibly NULL) pointer to destination
// Datum object, and flags.  If pDatum is NULL, create a new Datum object (which can be retrieved by the caller from the DFab
// record).  If the FabTrack flag is set, create a tracked object; otherwise, untracked.  If pDatum is not NULL and the
// FabPrepend or FabAppend flag is set, keep existing string value in pDatum (and prepend or append to it); otherwise, clear it.
// Return status code.
#if DCaller
static int dprep(DFab *pFab, Datum *pDatum, ushort flags, const char *caller, const char *dName) {
#else
static int dprep(DFab *pFab, Datum *pDatum, ushort flags) {
#endif

#if DDebug
	fprintf(logfile, "*dprep(...,%.8X,%s, ...): BEGIN\n", (uint) pDatum, (flags & FabAppend) ? "append" :
	 (flags & FabPrepend) ? "prepend" : "clear");
#endif
	pFab->stack = NULL;
	pFab->buf = NULL;

	// Initialize Datum object.
	if(pDatum == NULL) {
#if DCaller
		if(dmake(&pDatum, flags & FabTrack, "dprep", "pDatum") != 0)
#else
		if(dmake(&pDatum, flags & FabTrack) != 0)
#endif
			return -1;
		goto SetNull;
		}
	else if((flags & FabModeMask) == FabClear || !(pDatum->type & DStrMask)) {
		if(pDatum->type != dat_nil)
			flags = (flags & ~FabModeMask) | FabClear;	// Not string or nil: force "clear" mode.
SetNull:
		dsetnull(pDatum);
		}

	pFab->pDatum = pDatum;
	pFab->flags = flags;

	// Initialize fabrication buffers, preserving existing string value in Datum object if appending or prepending.
	if((flags & FabModeMask) != FabClear) {
		size_t used = strlen(pDatum->str);
		if(used < ChunkSizeMax) {
			if(fabGrow(pFab, used) != 0)			// Allocate a work buffer.
				return -1;
			if(used > 0) {					// If existing string not null...
				fabCopy(pFab, pDatum->str, used);	// copy it to work buffer, left or right justified...
				dsetnull(pDatum);			// and set Datum object to null.
				}
#if DDebug
			goto DumpIt;
#else
			return 0;
#endif
			}

		// Appending or prepending to existing string >= ChunkSizeMax in length.  Save it in a DChunk object and
		// re-initialize Datum object (without calling free()).
		if(fabSave(pDatum->str, used, pFab) != 0)
			return -1;
		dinit(pDatum);
		dsetnull(pDatum);
		}
#if DDebug
	if(fabGrow(pFab, 0) != 0)
		return -1;
DumpIt:
#if DCaller
	fprintf(logfile, "dprep(): Initialized Fab %.8X for %s(), var '%s': bufCur %.8X, buf %.8X\n", (uint) pFab, caller,
	 dName, (uint) pFab->bufCur, (uint) pFab->buf);
#else
	fprintf(logfile, "dprep(): Initialized Fab %.8X: bufCur %.8X, buf %.8X\n", (uint) pFab, (uint) pFab->bufCur,
	 (uint) pFab->buf);
#endif
	ddump(pDatum, "dprep(): post-Fab-init...");
	fflush(logfile);
	return 0;
#else
	return fabGrow(pFab, 0);
#endif
	}

// Open a fabrication object via dprep().  An untracked Datum object will be created.  Return status code.
#if DCaller
int dopen(DFab *pFab, const char *caller, const char *dName) {
#else
int dopen(DFab *pFab) {
#endif
	return dprep(pFab, NULL, FabClear);
	}

// Open a fabrication object via dprep().  A tracked Datum object will be created.  Return status code.
#if DCaller
int dopentrack(DFab *pFab, const char *caller, const char *dName) {
#else
int dopentrack(DFab *pFab) {
#endif
	return dprep(pFab, NULL, FabTrack | FabClear);
	}

// Open a fabrication object via dprep() with given Datum object (or NULL) and append flag.  Return status code.
#if DCaller
int dopenwith(DFab *pFab, Datum *pDatum, ushort mode, const char *caller, const char *dName) {
#else
int dopenwith(DFab *pFab, Datum *pDatum, ushort mode) {
#endif
	return (pDatum == NULL) ? emsg(-1, "Datum pointer cannot be NULL") : dprep(pFab, pDatum, mode);
	}

// Return true if a fabrication object is empty; otherwise, false.
bool disempty(const DFab *pFab) {

	return pFab->bufCur == (((pFab->flags & FabModeMask) == FabPrepend) ? pFab->bufEnd : pFab->buf) &&
	 pFab->stack == NULL;
	}

// End a fabrication object write operation and convert target Datum object to a string or blob, according to 'type'.  Return
// status code.
int dclose(DFab *pFab, int type) {

	// If still at starting point (no bytes written), change Datum object to empty blob if force; otherwise, leave as a
	// null string.
	if(disempty(pFab)) {
		if(type == Fab_Blob) {
			dsetblobref(NULL, 0, pFab->pDatum);
			pFab->pDatum->type = dat_blob;
			}
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
			bool isBinary = (memchr(str, '\0', len) != NULL);

			if(isBinary && type == Fab_String)
				goto BinErr;
			if(((isBinary || type == Fab_Blob) ? dsetblob((void *) str, len, pFab->pDatum) :
			 dsetsubstr(str, len, pFab->pDatum)) != 0)
				return -1;
			free((void *) pFab->buf);
			}
		else {
			char *buf0;

			// Not first chunk.  Add last one (which can't be empty) to stack and convert to a solo string or blob.
			// Remember pointer to work buffer in buf0 if prepending and work buffer is not full so that pointer can
			// be passed to free() later instead of 'str'.
			buf0 = (str != pFab->buf) ? pFab->buf : NULL;
			if(fabSave(str, len, pFab) != 0)
				return -1;
			else {
				DChunk *pChunk;
				size_t size;
				char *str0;
				bool isBinary;
				Datum *pDatum = pFab->pDatum;
				char workBuf[sizeof(DBlob)];

				// Get total length.
				len = 0;
				pChunk = pFab->stack;
#if DDebug
	fputs("    CHUNK LIST:\n", logfile);
#endif
				do {
					len += pChunk->blob.size;
#if DDebug
	fputs("\t\"", logfile);
	fvizs(pChunk->blob.mem, pChunk->blob.size, logfile);
	fputs("\"\n", logfile);
#endif
					} while((pChunk = pChunk->next) != NULL);

				// Get heap space.
				if(salloc(&str, len + 1, workBuf) != 0)
					return -1;
				str0 = str;

				// Copy string pieces into it and check for any imbedded null bytes.
				isBinary = false;
				pChunk = pFab->stack;
				if((pFab->flags & FabModeMask) == FabPrepend) {

					// Prepending: copy chunks from beginning to end.
					do {
						size = pChunk->blob.size;
						if(!isBinary && memchr(pChunk->blob.mem, '\0', size) != NULL)
							isBinary = true;
						str = memstpcpy((void *) str, (void *) pChunk->blob.mem, size);
						if(buf0 != NULL) {
							free((void *) buf0);
							buf0 = NULL;
							}
						else
							free((void *) pChunk->blob.mem);
						} while((pChunk = pChunk->next) != NULL);
					*str = '\0';
					}
				else {
					// Not prepending: copy chunks from end to beginning.
					str += len + 1;
					*--str = '\0';
					do {
						size = pChunk->blob.size;
						if(!isBinary && memchr(pChunk->blob.mem, '\0', size) != NULL)
							isBinary = true;
						str -= size;
						memcpy((void *) str, (void *) pChunk->blob.mem, size);
						free((void *) pChunk->blob.mem);
						} while((pChunk = pChunk->next) != NULL);
					}

				// Set byte string in Datum object if able.
				if(isBinary && type == Fab_String)
BinErr:
					return emsg(-1, "Cannot convert binary data to string");
				if(isBinary || type == Fab_Blob) {
					if(str0 == workBuf)
						return dsetblob((void *) str0, len, pDatum);
					dsetblobref((void *) str0, len, pDatum);
					pDatum->type = dat_blob;
					}
				else if(str0 == workBuf) {
					dsetnull(pDatum);
					strcpy(pDatum->u.miniStr, workBuf);
					}
				else
					dsetheapstr(str0, pDatum);
				}
			}
		}
#if DDebug
	ddump(pFab->pDatum, "dclose(): Finalized string...");
#endif
	return 0;
	}

// Stop tracking a Datum object by removing it from the garbage collection stack (if present).
void duntrack(Datum *pDatum) {

	if(pDatum == datGarbHead)
		datGarbHead = datGarbHead->next;
	else
		for(Datum *pDatum1 = datGarbHead; pDatum1 != NULL; pDatum1 = pDatum1->next)
			if(pDatum1->next == pDatum) {
				pDatum1->next = pDatum->next;
				pDatum->next = NULL;
				break;
				}
	}

// Copy one value to another.  Return status code.
int datcpy(Datum *pDest, const Datum *pSrc) {

	switch(pSrc->type) {
		case dat_nil:
		case dat_false:
		case dat_true:
			dclear(pDest);
			pDest->type = pSrc->type;
			pDest->str = NULL;
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
		case dat_soloStr:
		case dat_soloStrRef:
			return dsetstr(pSrc->str, pDest);
		case dat_blob:
			return dsetblob(pSrc->u.blob.mem, pSrc->u.blob.size, pDest);
		default:	// dat_blobRef
			dsetblobref(pSrc->u.blob.mem, pSrc->u.blob.size, pDest);
		}
	return 0;
	}

// Compare one Datum to another and return true if values are equal; otherwise, false.  If 'ignore' is true, ignore case in
// string comparisons.
bool dateq(const Datum *pDatum1, const Datum *pDatum2, bool ignore) {

	switch(pDatum1->type) {
		case dat_nil:
		case dat_false:
		case dat_true:
			return pDatum2->type == pDatum1->type;
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
		case dat_soloStr:
		case dat_soloStrRef:
			if(pDatum2->type & DStrMask) {
				int (*cmp)(const char *str1, const char *str2) = ignore ? strcasecmp : strcmp;
				return cmp(pDatum1->str, pDatum2->str) == 0;
				}
			return false;
		}

	// dat_blob, dat_blobRef
	return (pDatum2->type & DBlobMask) && pDatum1->u.blob.size == pDatum2->u.blob.size &&
	 memcmp(pDatum1->u.blob.mem, pDatum2->u.blob.mem, pDatum1->u.blob.size) == 0;
	}

// Delete given Datum object.  It assumed that either the object was permanent and not in the garbage collection list, or the
// caller is removing it from the list; for example, when called from datGarbPop().
#if DDebug
#if DCaller
void ddelete(Datum *pDatum, const char *caller) {
#else
void ddelete(Datum *pDatum) {
#endif
	ddump(pDatum, "ddelete(): About to free()...");
	fprintf(logfile, "ddelete(): calling dclear(%.8X)", (uint) pDatum);
#if DCaller
	fprintf(logfile, " on object from %s()", caller);
#endif
	fputs("...\n", logfile);
	fflush(logfile);
	dclear(pDatum);
	fprintf(logfile, "ddelete(): calling free(%.8X)... ", (uint) pDatum);
	fflush(logfile);
	free((void *) pDatum);
	fputs("done.\n", logfile); fflush(logfile);
#else
void ddelete(Datum *pDatum) {

	dclear(pDatum);
	free((void *) pDatum);
#endif
	}

// Pop datGarbHead to given pointer, releasing heap space and laughing all the way.
void datGarbPop(const Datum *pDatum) {
	Datum *pDatum1;

#if DDebug
	ddump(pDatum, "datGarbPop(): Popping to...");
	while(datGarbHead != pDatum) {
		if((pDatum1 = datGarbHead) == NULL) {
			fputs("datGarbHead is NULL! Bailing out...\n", logfile);
			break;
			}
		datGarbHead = datGarbHead->next;
#if DCaller
		ddelete(pDatum1, "datGarbPop");
#else
		ddelete(pDatum1);
#endif
		}
	fputs("datGarbPop(): END\n", logfile);
	fflush(logfile);
#else
	while(datGarbHead != pDatum) {
		pDatum1 = datGarbHead;
		datGarbHead = datGarbHead->next;
		ddelete(pDatum1);
		}
#endif
	}

// Copy a character to 'pFab' (an active fabrication object) in visible form.  Pass 'flags' to vizc().  Return status code.
int dputvizc(short c, ushort flags, DFab *pFab) {
	char *str;

	return (str = vizc(c, flags)) == NULL || dputs(str, pFab) != 0 ? -1 : 0;
	}

// Copy bytes from 'mem' to 'pFab' (an active fabrication object), exposing all invisible characters.  Pass 'flags' to vizc().
// If 'size' is zero, assume 'mem' is a null-terminated string; otherwise, copy exactly 'size' bytes.  Return status code.
int dputvizmem(const void *mem, size_t size, ushort flags, DFab *pFab) {
	char *mem1 = (char *) mem;
	size_t n = (size == 0) ? strlen(mem1) : size;

	for(; n > 0; --n)
		if(dputvizc(*mem1++, flags, pFab) != 0)
			return -1;
	return 0;
	}

// Copy bytes from 'mem' to 'pDatum' via dputvizmem().  Return status code.
int dsetvizmem(const void *mem, size_t size, ushort flags, Datum *pDatum) {
	DFab dest;

	return (dopenwith(&dest, pDatum, false) != 0 || dputvizmem(mem, size, flags, &dest) != 0 ||
	 dclose(&dest, Fab_String) != 0) ? -1 : 0;
	}

// Copy string from 'str' to 'pDatum', adding a single quote (') at beginning and end and converting single quote characters
// to '\''.  Return status.
int dshquote(const char *str, Datum *pDatum) {
	ptrdiff_t len;
	const char *str0, *str1;
	bool apostrophe;
	DFab dest;

	str0 = str;
	if(dopenwith(&dest, pDatum, false) != 0)
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
			if(dputmem("\\'", 2, &dest) != 0)
				return -1;
			}					// Copy chunk.
		else if(dputc('\'', &dest) != 0 || dputmem(str1, len, &dest) != 0 || dputc('\'', &dest) != 0)
			return -1;
		}
	if(*str0 == '\0' && dputmem("''", 2, &dest) != 0)	// Wrap it up.
		return -1;
	return dclose(&dest, Fab_String);
	}

// Return a Datum object as a string, if possible.  If not, set an error and return NULL.
char *dtos(const Datum *pDatum, bool vizNil) {
	static char workBuf[64];

	switch(pDatum->type) {
		case dat_nil:
			return vizNil ? "nil" : "";
		case dat_false:
			return "false";
		case dat_true:
			return "true";
		case dat_int:
			sprintf(workBuf, "%ld", pDatum->u.intNum);
			break;
		case dat_uint:
			sprintf(workBuf, "%lu", pDatum->u.uintNum);
			break;
		case dat_real:
			sprintf(workBuf, "%lf", pDatum->u.realNum);
			break;
		case dat_miniStr:
		case dat_soloStr:
		case dat_soloStrRef:
			return pDatum->str;
		case dat_blob:
		case dat_blobRef:
			(void) emsg(-1, "Cannot convert blob to string");
			return NULL;
		}
	return workBuf;
	}
