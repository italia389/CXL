// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// fastio.h		Header file for fast data I/O routines.

#ifndef fastio_h
#define fastio_h

#include "stdos.h"
#include <stdio.h>

// Definitions for fast data I/O routines.
typedef struct {
	int fileHandle;			// File handle/descriptor.
	ushort flags;			// File flags.
	const char *filename;		// Filename.
	char *dataBuf;			// Data buffer.
	char *dataBufCur;		// Current position in dataBuf.
	char *dataBufEnd;		// Static pointer to end of dataBuf if output; pointer to end of bytes read if input.
	size_t dataBufSize;		// Size of dataBuf.
	char *lineBuf;			// Input line buffer, used for data-sensitive reads and file slurping.
	char *lineBufCur;		// Pointer to next byte to store in lineBuf.
	char *lineBufEnd;		// Static pointer to end of lineBuf.
	short inpDelim1, inpDelim2;	// Input line delimiters (chars), or -1 if not defined.
	} FastFile;

#define FFOtpMode	0x0001		// File is opened for output; otherwise, input.
#define FFStdInp	0x0002		// File handle is standard input.
#define FFStdOtp	0x0004		// File handle is standard output.
#define FFSlurp		0x0008		// Slurping input file.
#define FFEOF		0x0010		// Input file is at EOF.

// Function declarations.
extern bool ffchomp(FastFile *pFastFile);
extern int ffclose(FastFile *pFastFile);
extern int ffclosekeep(FastFile *pFastFile);
extern int ffflush(FastFile *pFastFile);
extern void fffree(FastFile *pFastFile);
extern short ffgetc(FastFile *pFastFile);
extern ssize_t ffgets(FastFile *pFastFile);
extern FastFile *ffopen(const char *filename, short mode);
extern int ffprintf(FastFile *pFastFile, const char *fmt, ...);
extern int ffputc(short c, FastFile *pFastFile);
extern int ffputs(const char *str, FastFile *pFastFile);
extern ssize_t ffread(void *buf, ssize_t len, FastFile *pFastFile);
extern int ffSetDelim(const char *delimType, FastFile *pFastFile);
extern ssize_t ffslurp(FastFile *pFastFile);
extern int ffvizc(short c, ushort flags, FastFile *pFastFile);
extern int ffvizmem(const void *mem, size_t size, ushort flags, FastFile *pFastFile);
extern int ffwrite(void *buf, size_t len, FastFile *pFastFile);
#endif
