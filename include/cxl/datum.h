// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// datum.h		Header file for creating and managing Datum objects.

#ifndef datum_h
#define datum_h

#include "stdos.h"

// Definitions for creating and managing strings, integers, and other data types allocated from heap space.

// Definitions for dopenwith() function.
#define FabClear	0			// Clear data in caller's Datum object.
#define FabAppend	1			// Append to caller's Datum object.
#define FabPrepend	2			// Prepend to caller's Datum object.
#define FabModeMask	0x0003			// Bits for mode value.

// Definitions for debugging.
#define DDebug		0			// Include debugging code for pDatum objects.
#define DCaller		0			// Use "caller-tracking" code and sanity checks for pDatum objects.
#define DFabTest	0			// Include fabrication test code.

// Blob object: used for holding generic chunks of memory, such as a struct or byte string.
typedef struct {
	size_t size;				// Size in bytes.
	void *mem;
	} DBlob;

// Chunk object: used for holding chunks of memory for fabrication (DFab) objects.
typedef struct DChunk {
	struct DChunk *next;			// Link to next item in list.
	DBlob blob;				// Blob object.
	} DChunk;

// Datum object: general purpose structure for holding a nil value, Boolean value, signed or unsigned long integer, real number,
// string of any length, or blob.
typedef ushort DatumType;
typedef struct Datum {
	struct Datum *next;			// Link to next item in list.
	DatumType type;				// Type of value.
	char *str;				// String value if dat_miniStr, dat_soloStr, or dat_soloStrRef; otherwise, NULL.
	union {					// Current value.
		long intNum;			// Signed integer.
		ulong uintNum;			// Unsigned integer.
		double realNum;			// Real number.
		char miniStr[sizeof(DBlob)];	// Self-contained mini-string.
		char *soloStr;			// Solo string (on heap).
		DBlob blob;			// Blob object.
		} u;
	} Datum;

// Datum types.
#define dat_nil		0x0000			// Nil value.
#define dat_false	0x0001			// False value.
#define dat_true	0x0002			// True value.
#define dat_int		0x0004			// Signed integer type.
#define dat_uint	0x0008			// Unsigned integer type.
#define dat_real	0x0010			// Real number (double) type.
#define dat_miniStr	0x0020			// Mini string.
#define dat_soloStr	0x0040			// Solo string by value.
#define dat_soloStrRef	0x0080			// Solo string by reference.
#define dat_blob	0x0100			// Blob object by value.
#define dat_blobRef	0x0200			// Blob object by reference.

#define DBoolMask	(dat_false | dat_true)				// Boolean types.
#define DStrMask	(dat_miniStr | dat_soloStr | dat_soloStrRef)	// String types.
#define DBlobMask	(dat_blob | dat_blobRef)			// Blob types.

// Fabrication object: used to build a string or blob in pieces, forward or backward.
typedef struct {
	Datum *pDatum;				// Datum pointer.
	DChunk *stack;				// Chunk stack (linked list).
	char *bufCur;				// Next byte to store in buf.
	char *bufEnd;				// Ending byte position in buf.
	char *buf;				// Work buffer.
	ushort flags;				// Operation mode.
	} DFab;

// Fabrication close types used by dclose().
#define Fab_String	-1			// String type (may not contain null bytes).
#define Fab_Auto	0			// Both string and blob types allowed.
#define Fab_Blob	1			// Blob type.

// External variables.
extern Datum *datGarbHead;			// Head of list of temporary Datum records ("garbage collection").

// External function declarations and aliases.
extern int datcpy(Datum *pDest, const Datum *pSrc);
extern bool dateq(const Datum *pDatum1, const Datum *pDatum2, bool ignore);
extern void datGarbPop(const Datum *pDatum);
extern Datum *datxfer(Datum *pDest, Datum *pSrc);
extern void dclear(Datum *pDatum);
extern int dclose(DFab *pFab, int type);
#if DCaller
extern void ddelete(Datum *pDatum, const char *caller);
#else
extern void ddelete(Datum *pDatum);
#endif
#if DDebug
extern void ddump(Datum *pDatum, char *label);
#endif
#if DCaller
extern void dinit(Datum *pDatum, const char *caller);
#else
extern void dinit(Datum *pDatum);
#endif
extern bool disempty(const DFab *pFab);
extern bool disfalse(const Datum *pDatum);
extern bool disnil(const Datum *pDatum);
extern bool disnull(const Datum *pDatum);
extern bool distrue(const Datum *pDatum);
#if DCaller
extern int dnew(Datum **ppDatum, const char *caller, const char *dName);
extern int dnewtrack(Datum **ppDatum, const char *caller, const char *dName);
extern int dopen(DFab *pFab, const char *caller, const char *dName);
extern int dopentrack(DFab *pFab, const char *caller, const char *dName);
extern int dopenwith(DFab *pFab, Datum *pDatum, ushort mode, const char *caller, const char *dName);
#else
extern int dnew(Datum **ppDatum);
extern int dnewtrack(Datum **ppDatum);
extern int dopen(DFab *pFab);
extern int dopentrack(DFab *pFab);
extern int dopenwith(DFab *pFab, Datum *pDatum, ushort mode);
#endif
extern int dputc(short c, DFab *pFab);
extern int dputd(const Datum *pDatum, DFab *pFab);
extern int dputf(DFab *pFab, const char *fmt, ...);
extern int dputmem(const void *mem, size_t size, DFab *pFab);
extern int dputs(const char *str, DFab *pFab);
extern int dputvizc(short c, ushort flags, DFab *pFab);
extern int dputvizmem(const void *mem, size_t size, ushort flags, DFab *pFab);
extern int dsalloc(Datum *pDatum, size_t len);
extern int dsetblob(const void *mem, size_t size, Datum *pDatum);
extern void dsetblobref(void *mem, size_t size, Datum *pDatum);
extern void dsetbool(bool b, Datum *pDatum);
extern void dsetchr(short c, Datum *pDatum);
extern void dsetheapstr(char *str, Datum *pDatum);
extern void dsetint(long i, Datum *pDatum);
#define dsetnil(pDatum)		dclear(pDatum)
extern void dsetnull(Datum *pDatum);
extern void dsetreal(double d, Datum *pDatum);
extern int dsetstr(const char *str, Datum *pDatum);
extern void dsetstrref(char *str, Datum *pDatum);
extern int dsetsubstr(const char *str, size_t len, Datum *pDatum);
extern void dsetuint(ulong u, Datum *pDatum);
extern int dsetvizmem(const void *mem, size_t size, ushort flags, Datum *pDatum);
extern int dshquote(const char *str, Datum *pDatum);
extern char *dtos(const Datum *pDatum, bool vizNil);
extern int dunputc(DFab *pFab);
extern void duntrack(Datum *pDatum);
#endif
