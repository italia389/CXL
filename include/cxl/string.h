// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// string.h		Header file for string extension routines.

#ifndef string_h
#define string_h

#include <stdio.h>
#include "cxl/datum.h"

// Definitions for vizc() routine.
#define VizBaseDef	0x0000		// Use default base (VizBaseHEX).
#define VizBaseOctal	0x0001		// Octal base.
#define VizBaseHex	0x0002		// Hex base (lower case letters).
#define VizBaseHEX	0x0003		// Hex base (upper case letters).
#define VizBaseMask	0x0003
#define VizBaseMax	VizBaseHEX

#define VizSpace	0x0004		// Make space character visible.
#define VizMask		0x000F

// Definitions for split() and join() routines.
typedef struct {
	uint itemCount;
	char *items[1];			// Array length is set when malloc() is called.
	} StrArray;

// External function declarations.
extern int join(Datum *pDest, StrArray *pSrc, const char *delim);
extern int memcasecmp(const void *str1, const void *str2, size_t len);
extern void *memstpcpy(void *dest, const void *src, size_t len);
extern StrArray *split(short delim, char *src, int limit);
extern char *stplcpy(char *dest, const char *src, size_t len);
extern char *strcbrk(const char *str, const char *charset);
extern int strconv(char *dest, const char *src, const char **pSrcEnd, short termChar);
extern char *strfit(char *dest, size_t maxLen, const char *src, size_t srcLen);
extern char *strip(char *str, int loc);
extern char *vizc(short c, ushort flags);
#endif
