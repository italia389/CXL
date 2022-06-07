// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// datum.h		Header file for creating and managing Datum objects.

#ifndef datum_h
#define datum_h

#include "stdos.h"

// Forwards.
struct Array;

// Definitions for creating and managing strings, integers, and other data types allocated from memory.

// Memory object: used for holding generic chunks of memory, such as a struct or byte string.
typedef struct {
	size_t size;				// Size in bytes.
	void *ptr;				// Memory pointer.
	} DMem;

// Chunk object: used for holding chunks of memory for fabrication (DFab) objects.
typedef struct DChunk {
	struct DChunk *next;			// Link to next item in list.
	DMem mem;				// Memory object.
	} DChunk;

// Datum object: general purpose structure for holding a nil value, Boolean value, signed or unsigned long integer, real number,
// string of any length, or byte string of any length.
typedef ushort DatumType;
typedef struct Datum {
	struct Datum *next;			// Link to next item in list (for garbage collection).
	DatumType type;				// Type of value.
	char *str;				// String value if dat_miniStr, dat_longStr, or dat_longStrRef, otherwise NULL.
	union {					// Current value.
		short c;			// Character value.
		long intNum;			// Signed integer.
		ulong uintNum;			// Unsigned integer.
		double realNum;			// Real number.
		char miniStr[sizeof(DMem)];	// Self-contained mini-string.
		char *longStr;			// String allocated on heap.
		DMem mem;			// Memory object.
		struct Array *pArray;		// Array object.
		} u;
	} Datum;

// Datum types.
#define dat_nil		0x0000			// Nil value.
#define dat_false	0x0001			// False value.
#define dat_true	0x0002			// True value.
#define dat_char	0x0004			// Character (int) value.
#define dat_int		0x0008			// Signed integer type.
#define dat_uint	0x0010			// Unsigned integer type.
#define dat_real	0x0020			// Real number (double) type.
#define dat_miniStr	0x0040			// Mini string.
#define dat_longStr	0x0080			// String by value.
#define dat_longStrRef	0x0100			// String by reference.
#define dat_byteStr	0x0200			// Byte string by value.
#define dat_byteStrRef	0x0400			// Byte string by reference.
#define dat_array	0x0800			// Array by value.
#define dat_arrayRef	0x1000			// Array by reference.

#define DBoolMask	(dat_false | dat_true)				// Boolean types.
#define DStrMask	(dat_miniStr | dat_longStr | dat_longStrRef)	// String types.
#define DMemMask	(dat_byteStr | dat_byteStrRef)			// Memory types.
#define DArrayMask	(dat_array | dat_arrayRef)			// Array types.

// Fabrication object: used to build a string or byte string in pieces, forward or backward.
typedef struct {
	Datum *pDatum;				// Datum pointer.
	DChunk *stack;				// Chunk stack (linked list).
	char *bufCur;				// Next byte to store in buf.
	char *bufEnd;				// Ending byte position in buf.
	char *buf;				// Work buffer.
	ushort flags;				// Operation mode.
	} DFab;

// Definitions for dopenwith() function.
#define FabClear	0			// Clear data in caller's Datum object.
#define FabAppend	1			// Append to caller's Datum object.
#define FabPrepend	2			// Prepend to caller's Datum object.
#define FabModeMask	0x0003			// Bits for mode value.

// For internal use.
#define FabTrack	0x0004			// Create tracked Datum object in fabrication object.

// Fabrication close types, used by dclose().
#define FabAuto		0x0000			// Automatically use FabStr (if able) or FabMem.
#define FabStr		0x0001			// String type (may not contain null bytes).
#define FabStrRef	0x0002			// String reference type (may not contain null bytes).
#define FabMem		0x0004			// Memory type.
#define FabMemRef	0x0008			// Memory reference type.

#define FabCloseMask	0x000F
#define FabCloseBits	4			// Number of close type bits in use (for validity checking).

// Flags for controlling datum conversions (cflags) for non-array datum types and array types.  These may be combined with
// vizc() flags (like VizBaseHex), so higher bits are used.
// Notes:
//  1. DCvtEscChar overrides DCvtVizChar.
//  2. DCvtQuote1 overrides DCvtQuote2 and DCvtQuote, and DCvtQuote2 overrides DCvtQuote.
//  3. If DCvtQuote is specified (and DCvtQuote1 and DCvtQuote2 are not), characters are always single-quoted, and strings are
//     single-quoted if DCvtVizChar (or any of the Viz* flags) is specified; otherwise, strings are double-quoted.
//  4. If no flags are specified, all datums are converted to "raw" string form; that is, without translation to visible form,
//     quotation marks, or delimiters between array elements, except that Booleans are always output as "true" and "false".
#define DCvtShowNil	0x0010			// Output "nil" for nil value, otherwise a null string.
#define DCvtVizChar	0x0020			// Output characters via vizc() calls, otherwise raw.
#define DCvtEscChar	0x0040			// Output characters in escaped (\r, \n, etc.) form, otherwise raw.
#define DCvtQuote1	0x0080			// Enclose characters and strings in single (') quotes.
#define DCvtQuote2	0x0100			// Enclose characters and strings in double (") quotes.
#define DCvtQuote	0x0200			// Quote characters and strings depending on type and flags.
#define DCvtThouSep	0x0400			// Insert commas in thousands' places in integers.
#define DCvtQuoteMask	(DCvtQuote1 | DCvtQuote2 | DCvtQuote)

#define ACvtBrkts	0x0800			// Wrap array elements in brackets [...].
#define ACvtDelim	0x1000			// Insert comma delimiter between array elements.
#define ACvtSkipNil	0x2000			// Skip nil arguments in arrays.

// Shortcut macros.
#define DCvtLang	(DCvtShowNil | DCvtEscChar | DCvtQuote | ACvtBrkts | ACvtDelim)
						// Convert all datum types to "language expression" form.
#define DCvtVizStr	(DCvtShowNil | DCvtVizChar | DCvtQuote1 | ACvtBrkts | ACvtDelim)
						// Convert all datum types to visible form via vizc() calls.

// For internal use.
#define ACvtNoBrkts	0x8000			// Suppress brackets "[...]" around array elements.

// Flags for controlling datum operations (dflags).  These may be combined with array operation flags (aflags).
#define DOpIgnore	AOpIgnore		// Ignore case in string comparisons.

// Macro shortcuts.
#define dischr(pDatum)		((pDatum)->type == dat_char)
#define disfalse(pDatum)	((pDatum)->type & (dat_false | dat_nil))
#define disnil(pDatum)		((pDatum)->type == dat_nil)
#define distrue(pDatum)		(!((pDatum)->type & (dat_false | dat_nil)))

#define dtyparray(pDatum)	((pDatum)->type & DArrayMask)
#define dtypbool(pDatum)	((pDatum)->type & DBoolMask)
#define dtypmem(pDatum)		((pDatum)->type & DMemMask)
#define dtypstr(pDatum)		((pDatum)->type & DStrMask)

#include "cxl/array.h"

// External variables.
extern Datum *datGarbHead;			// Head of list of temporary Datum records ("garbage collection").

// External function declarations and aliases.
extern void dadoptarray(struct Array *pArray, Datum *pDatum);
extern void dadoptmem(void *memPtr, size_t size, Datum *pDatum);
extern void dadoptstr(char *str, Datum *pDatum);

extern void dconvchr(short c, Datum *pDatum);
extern int dcpy(Datum *pDest, const Datum *pSrc);
extern bool deq(const Datum *pDatum1, const Datum *pDatum2, ushort dflags);
extern void dgarbpop(const Datum *pDatum);
extern void dclear(Datum *pDatum);
extern int dclose(DFab *pFab, ushort type);
extern bool dfabempty(const DFab *pFab);
extern void dfree(Datum *pDatum);
extern void dinit(Datum *pDatum);
extern bool disempty(const Datum *pDatum);
extern bool disnull(const Datum *pDatum);
extern int dnew(Datum **ppDatum);
extern int dnewtrack(Datum **ppDatum);
extern int dopen(DFab *pFab);
extern int dopentrack(DFab *pFab);
extern int dopenwith(DFab *pFab, Datum *pDatum, ushort mode);

extern int dputc(short c, DFab *pFab, ushort cflags);
extern int dputd(const Datum *pDatum, DFab *pFab, const char *delim, ushort cflags);
extern int dputf(DFab *pFab, ushort cflags, const char *fmt, ...);
extern int dputmem(const void *memPtr, size_t size, DFab *pFab, ushort cflags);
extern int dputs(const char *str, DFab *pFab, ushort cflags);
extern int dputsubstr(const char *str, size_t len, DFab *pFab, ushort cflags);

extern int drelease(Datum *pDatum);
extern int dsalloc(Datum *pDatum, size_t len);

extern int dsetarray(const struct Array *pArray, Datum *pDatum);
extern void dsetarrayref(struct Array *pArray, Datum *pDatum);
extern void dsetbool(bool b, Datum *pDatum);
extern void dsetchr(short c, Datum *pDatum);
extern void dsetint(long i, Datum *pDatum);
extern int dsetmem(const void *memPtr, size_t size, Datum *pDatum);
extern void dsetmemref(void *memPtr, size_t size, Datum *pDatum);
#define dsetnil(pDatum)		dclear(pDatum)
extern void dsetnull(Datum *pDatum);
extern void dsetreal(double d, Datum *pDatum);
extern int dsetstr(const char *str, Datum *pDatum);
extern void dsetstrref(char *str, Datum *pDatum);
extern int dsetsubstr(const char *str, size_t len, Datum *pDatum);
extern void dsetuint(ulong u, Datum *pDatum);

extern int dshquote(const char *str, Datum *pDatum);
extern int dtos(Datum *pDest, const Datum *pSrc, const char *delim, ushort cflags);
extern void dtrack(Datum *pDatum);
extern int dunputc(DFab *pFab);
extern void duntrack(Datum *pDatum);
extern void dxfer(Datum *pDest, Datum *pSrc);

// For internal use.
extern int bputc(short c, DFab *pFab);
extern int bputs(const char *str, DFab *pFab);
#endif
