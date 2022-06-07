// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// excep.c		Exception-handling routines and data.

#include "stdos.h"
#include "cxl/excep.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#define	DefExitCode	-1		// Value to use for exit() call if not available elsewhere.

// Global variables.
CXLExcep cxlExcep = {0, 0, ""};

// Free exception message allocated from heap, if applicable.
static void freeMsg(void) {

	if(cxlExcep.flags & ExcepHeap) {
		(void) free((void *) cxlExcep.msg);
		cxlExcep.flags &= ~ExcepHeap;
		}
	}

// Set an exception code and message, freeing old message if it was allocated from heap.  Return status code.
int emsg(int code, const char *msg) {

	freeMsg();
	cxlExcep.msg = (char *) msg;
	return cxlExcep.code = code;
	}

// Set an exception code and system (errno) message.  Return status code.
int emsgsys(int code) {

	return emsg(code, strerror(errno));
	}

// Set an exception code and formatted message, freeing old message if it was allocated from heap.  Return status code.
int emsgf(int code, const char *fmt, ...) {
	va_list varArgList;

	freeMsg();
	va_start(varArgList, fmt);
	(void) vasprintf(&cxlExcep.msg, fmt, varArgList);
	va_end(varArgList);
	if(cxlExcep.msg == NULL) {
		cxlExcep.msg = strerror(errno);
		return cxlExcep.code = -1;
		}
	cxlExcep.flags |= ExcepHeap;
	return cxlExcep.code = code;
	}

// Handle program exception; routine's behavior is controlled by supplied flag word.  Flag bit masks are:
//
//   ExAbort		Abort message, call exit().
//   ExError		Error message, call exit().
//   ExWarning		Warning message, do not call exit(), return non-zero.
//   ExNotice		Notice message, do not call exit(), return non-zero.
//
//   ExErrNo		errno variable contains exception code.
//   ExMessage		cxlExcep.msg variable contains exception message.
//   ExCustom		Formatted message.
//
//   ExExit		Force exit() call.
//   ExNoExit		Skip exit() call, return non-zero.
//
// Notes:
//  1. Flags in severity and exit groups are mutally exclusive.
//  2. Routine builds and prints exception message to standard error and, optionally, exits.
//  3. Arguments are processed as follows:
//	1. Print message prefix per severity flag in first argument, if specified.
//	2. If ExMessage flag is set, print cxlExcep.msg; otherwise, if ExErrNo flag is set, print strerror(errno) message.
//	3. If ExCustom flag is set, call printf() with remaining arguments.
//	4. If ExErrNo flag is set and errno is non-zero, use errno for exit code; otherwise, use EXIT_FAILURE.
//	5. Call exit() if applicable; otherwise, return DefExitCode.
int excep(uint flags, ...) {
	static struct {
		bool callExit;
		char *severityPrefix;
		} severityTable[] = {
			{false, NULL},
			{false, "Notice"},
			{false, "Warning"},
			{true, "Error"},
			{true, "Abort"}
			};
	uint severityIndex;
	bool callExit;
	bool needComma = false;

	// Print severity prefix if requested and not out of range.
	if((severityIndex = flags & ExSeverityMask) != 0) {
		if(severityIndex < elementsof(severityTable))
			fprintf(stderr, "%s: ", severityTable[severityIndex].severityPrefix);
		else
			severityIndex = 0;
		}

	// Check for library or errno message and print it if requested.
	if(flags & (ExMessage | ExErrNo)) {
		fputs((flags & ExMessage) ? cxlExcep.msg : strerror(errno), stderr);
		needComma = true;
		}

	// Print formatted message, if requested.
	if(flags & ExCustom) {
		va_list varArgList;
		char *fmt;

		va_start(varArgList, flags);
		if(needComma)
			fputs(", ", stderr);
		fmt = va_arg(varArgList, char *);
		vfprintf(stderr, fmt, varArgList);
		va_end(varArgList);
		}

	// Finish up.
	fputc('\n', stderr);

	// Call exit(), if applicable.
	callExit = severityTable[severityIndex].callExit;
	switch(flags & ExExitMask) {
	case ExExit:
		callExit = true;
		break;
	case ExNoExit:
		callExit = false;
	}
	if(callExit)
		exit((flags & ExErrNo) && errno ? errno : EXIT_FAILURE);

	return DefExitCode;
	}
