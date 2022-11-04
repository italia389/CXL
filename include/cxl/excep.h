// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// excep.h		Header file for CXL library exception handling.

#ifndef excep_h
#define excep_h

#include "stdos.h"

// Definitions for library exception handling.
typedef struct {
	int code;			// Exception code: -1 (error) or 0 (success).
	ushort flags;			// Exception message flags.
	char *msg;			// Exception message.
	} CXLExcep;

#define ExcepHeap	0x0001		// Message was allocated from heap.
#define ExcepMem	0x0002		// Error in memory-allocation function; e.g., malloc().

// Definitions for excep() function.
#define ExNotice	1		// Display "Notice".
#define ExWarning	2		// Display "Warning".
#define ExError		3		// Display "Error".
#define ExAbort		4		// Display "Abort".
#define ExSeverityMask	0x0007		// Severity bit mask.

#define ExMessage	0x0008		// 'cxlExcep.msg' variable contains exception message.
#define ExErrNo		0x0010		// 'errno' variable contains exception code.
#define ExCustom	0x0020		// Display custom, formatted message.

#define ExExit		0x0040		// Force exit() call.
#define ExNoExit	0x0080		// Skip exit() call.
#define ExExitMask	(ExExit | ExNoExit)	// Exit flags.

// External variables.
extern CXLExcep cxlExcep;

// External function declarations.
extern void eclear(void);
extern int emsg(int code, const char *msg);
extern int emsgf(int code, const char *fmt, ...);
extern int emsgsys(int code);
extern int excep(uint flags, ...);
#endif
