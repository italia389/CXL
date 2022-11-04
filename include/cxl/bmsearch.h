// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// bmsearch.h		Header file for Boyer-Moore search routines.

#ifndef bmsearch_h
#define bmsearch_h

#include "stdos.h"

// Definitions for compiling and executing a pattern.
#define CharSetLen		256		// Total number of 8-bit characters.

// Compilation flags.
#define PatIgnoreCase		0x0001		// Ignore case.
#define PatReverse		0x0002		// Search backward, otherwise forward.

typedef ssize_t CharOffset;		// Arithmetic type of pattern and text character offsets.
typedef struct {
	CharOffset *badChar;		// "Bad character" jump table.
	CharOffset *goodSuf;		// "Good suffix" jump table.
	CharOffset *suff;
	char *lcPat;			// Pattern in lower case if PatIgnoreCase flag set, otherwise NULL.
	} BMData;

typedef struct {
	unsigned cflags;		// Compilation flags.
	char *pat;			// Null-terminated pattern (reversed if PatReverse flag set).
	CharOffset len;			// Pattern length.
	BMData data;			// Internal data.
	} BMPat;

// External function declarations.
extern int bmcomp(BMPat *pPat, const char *pat, unsigned cflags);
extern int bmncomp(BMPat *pPat, const char *pat, CharOffset len, unsigned cflags);
extern const char *bmexec(const BMPat *pPat, const char *string);
extern const char *bmnexec(const BMPat *pPat, const char *string, size_t len);
extern CharOffset bmuexec(const BMPat *pPat, bool (*nextChar)(short *pc, void *context), void *context);
extern void bmfree(BMPat *pPat);
#endif
