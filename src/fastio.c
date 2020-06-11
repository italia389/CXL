// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// fastio.c		Fast data I/O routines.

#include "stdos.h"
#include "cxl/excep.h"
#include "cxl/string.h"
#include "cxl/fastio.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define StdInBufSize		16384		// File buffer size for standard input.
#define FileBufSize		65536		// Buffer size for all other files.
#define LineBufSize		256		// Initial line buffer size.
#define LineBufMult		4		// Multiplier for growing line buffer when full.

// Return number of unread data bytes in file buffer.  If empty, read more.  If an error occurs, return -1.
static ssize_t fileBufSize(FastFile *pFastFile) {
	ssize_t bufBytes;

	// At EOF?
	if(pFastFile->flags & FFEOF)
		return 0;

	// Any unread bytes left in file buffer?
	if((bufBytes = pFastFile->dataBufEnd - pFastFile->dataBufCur) == 0) {

		// Nope, read more.
		if((bufBytes = read(pFastFile->fileHandle, (void *) (pFastFile->dataBufCur = pFastFile->dataBuf),
		 pFastFile->dataBufSize)) == -1)
			return emsgf(-1, "%s, reading %lu bytes from file '%s'", strerror(errno), pFastFile->dataBufSize,
			 pFastFile->filename);
		pFastFile->dataBufEnd = pFastFile->dataBuf + bufBytes;
		if(bufBytes == 0) {
			pFastFile->flags |= FFEOF;
			return 0;
			}
		}

	return bufBytes;
	}

// Flush output buffer.  Return status code.
int ffflush(FastFile *pFastFile) {
	ssize_t bytesToWrite, bytesWritten;

	if((bytesToWrite = pFastFile->dataBufCur - pFastFile->dataBuf) > 0) {
		if((bytesWritten = write(pFastFile->fileHandle, (void *) pFastFile->dataBuf, bytesToWrite)) == -1)
			return emsgf(-1, "%s, writing %ld bytes to file '%s'", strerror(errno), bytesToWrite,
			 pFastFile->filename);
		if(bytesWritten != bytesToWrite)
			return emsgf(-1, "Short file write (attempted %ld bytes, wrote %ld), file '%s'",
			 bytesToWrite, bytesWritten, pFastFile->filename);

		// Reset buffer pointer.
		pFastFile->dataBufCur = pFastFile->dataBuf;
		}
	return 0;
	}

// Free file object.
void fffree(FastFile *pFastFile) {

	// Release heap space.
	if(pFastFile->lineBuf != NULL)
		free((void *) pFastFile->lineBuf);
	free((void *) pFastFile);
	}

// Close a data file, keeping buffers intact.  Caller must call fffree() when file object is no longer needed.  Return status
// code.
int ffclosekeep(FastFile *pFastFile) {

	// Flush if output file.
	if((pFastFile->flags & FFOtpMode) && ffflush(pFastFile) != 0)
		return -1;

	// Close file, checking for standard input or output.  Return exception message if error.
	if((pFastFile->flags & FFStdInp) ? fclose(stdin) != 0 : (pFastFile->flags & FFStdOtp) ? fclose(stdout) != 0 :
	 close(pFastFile->fileHandle) == -1)
		return emsgf(-1, "%s, closing file '%s'", strerror(errno), pFastFile->filename);

	return 0;
	}

// Close a data file and release all heap space.  Return status code.
int ffclose(FastFile *pFastFile) {

	if(ffclosekeep(pFastFile) != 0)
		return -1;
	fffree(pFastFile);
	return 0;
	}

// Read a character from a data file and return it, or -1 if end of file.  If an error occurs, return -2.
short ffgetc(FastFile *pFastFile) {

	// At EOF?
	if(pFastFile->flags & FFEOF)
		return -1;

	// Any bytes left in buffer?
	if(pFastFile->dataBufCur == pFastFile->dataBufEnd) {
		ssize_t bufBytes;

		// Nope.  Get more and check for EOF or error.
		if((bufBytes = fileBufSize(pFastFile)) == 0)
			return -1;
		if(bufBytes < 0)
			return -2;
		}

	// Return next byte.
	return *pFastFile->dataBufCur++;
	}

// Grow line buffer.  Use LineBufSize if first allocation; otherwise, set new size to <old-size> * LineBufMult.
// Return status code.
static int growLineBuf(FastFile *pFastFile) {
	size_t oldBufSize, newBufSize, used;

	// Get buffer sizes.
	oldBufSize = (pFastFile->lineBuf == NULL) ? 0 : pFastFile->lineBufEnd - pFastFile->lineBuf;
	newBufSize = oldBufSize == 0 ? LineBufSize : oldBufSize * LineBufMult;
	used = (pFastFile->lineBuf == NULL) ? 0 : pFastFile->lineBufCur - pFastFile->lineBuf;
	if((pFastFile->lineBuf = (char *) realloc((void *) pFastFile->lineBuf, newBufSize)) == NULL) {
		cxlExcep.flags |= ExcepMem;
		return emsgf(-1, "%s, allocating %lu-byte buffer for file '%s'", strerror(errno), newBufSize,
		 pFastFile->filename);
		}
	pFastFile->lineBufCur = pFastFile->lineBuf + used;
	pFastFile->lineBufEnd = pFastFile->lineBuf + newBufSize;

	return 0;
	}

// Open data file, given filename and mode.  Mode must be one of the following:
//	'r'	Attach file handle to standard input if "filename" is NULL or "-"; otherwise, open file for input.
//	'w'	Attach file handle to standard output if "filename" is NULL or "-"; otherwise, open file for output.
//	'a'	Attach file handle to standard output if "filename" is NULL or "-"; otherwise, open file for appending.
// If open is successful, a pointer to a FastFile object is returned; otherwise, an exception message is set and NULL is
// returned.
FastFile *ffopen(const char *filename, short mode) {
	FastFile *pFastFile;
	int handle, openFlags;
	ushort flags;
	size_t dataBufSize = FileBufSize;
	bool inputMode = false;

	// Check mode.
	switch(mode) {
		case 'r':
			inputMode = true;
			if(filename == NULL || strcmp(filename, "-") == 0) {
				handle = fileno(stdin);
				filename = "(stdin)";
				flags = FFStdInp;
				dataBufSize = StdInBufSize;
				break;
				}
			openFlags = O_RDONLY;
			flags = 0;
			goto Open;
		case 'w':
			openFlags = O_WRONLY | O_CREAT | O_TRUNC;
			goto Output;
		case 'a':
			openFlags = O_WRONLY | O_CREAT | O_APPEND;
Output:
			if(filename == NULL || strcmp(filename, "-") == 0) {
				handle = fileno(stdout);
				filename = "(stdout)";
				flags = FFStdOtp | FFOtpMode;
				break;
				}
			flags = FFOtpMode;
Open:
			// Open file.
			if((handle = open(filename, openFlags, 0664)) == -1)
				goto Err;
			break;
		default:
			(void) emsgf(-1, "Unknown mode '%s', opening file '%s'", vizc(mode, 0), filename);
			return NULL;
		}

	// Allocate FastFile object and initialize it.
	if((pFastFile = (FastFile *) malloc(sizeof(FastFile) + dataBufSize)) == NULL) {
Err:
		cxlExcep.flags |= ExcepMem;
		(void) emsgf(-1, "%s, opening file '%s' with mode '%c'", strerror(errno), filename, mode);
		return NULL;
		}
	pFastFile->fileHandle = handle;
	pFastFile->filename = filename;
	pFastFile->dataBufSize = dataBufSize;
	pFastFile->dataBufCur = pFastFile->dataBuf = (char *)(pFastFile + 1);
	pFastFile->lineBuf = NULL;
	pFastFile->inpDelim1 = pFastFile->inpDelim2 = -1;
	if(inputMode) {
		pFastFile->dataBufEnd = pFastFile->dataBuf;		// Empty input buffer.

		// Allocate initial line buffer.
		if(growLineBuf(pFastFile) != 0)
			return NULL;
		*pFastFile->lineBufCur = '\0';
		}
	else
		pFastFile->dataBufEnd = pFastFile->dataBuf + dataBufSize;
	pFastFile->flags = flags;
	return pFastFile;
	}

// Set data-sensitive delimiter type ("cr", "crlf", or "nl") on an input file.  Return status code.
int ffSetDelim(const char *delimType, FastFile *pFastFile) {

	// Error if output file.
	if(pFastFile->flags & FFOtpMode)
		return emsgf(-1, "Cannot set a delimiter type on output file '%s'", pFastFile->filename);

	// Convert type to actual delimiters.
	pFastFile->inpDelim2 = -1;
	if(strcmp(delimType, "cr") == 0)
		pFastFile->inpDelim1 = '\r';
	else if(strcmp(delimType, "crlf") == 0) {
		pFastFile->inpDelim1 = '\r';
		pFastFile->inpDelim2 = '\n';
		}
	else if(strcmp(delimType, "nl") == 0)
		pFastFile->inpDelim1 = '\n';
	else
		return emsgf(-1, "Invalid delimiter type '%s' for file '%s' (must be \"cr\", \"crlf\", or \"nl\")", delimType,
		 pFastFile->filename);
	return 0;
	}

// Write a character to a data file.  Return status code.
int ffputc(short c, FastFile *pFastFile) {
	int rtnCode;

	// Buffer full?
	if(pFastFile->dataBufCur == pFastFile->dataBufEnd && (rtnCode = ffflush(pFastFile)) != 0)
		return rtnCode;

	// Store character in buffer.
	*pFastFile->dataBufCur++ = c;

	return 0;
	}

// Read "len" bytes from a data file into the given buffer (which must be a minimum of "len" bytes in size).  A terminating null
// byte is *not* written.  Return number of bytes read or -1 if error.  If EOF is reached, zero is returned.
ssize_t ffread(void *buf, ssize_t len, FastFile *pFastFile) {
	size_t bytesLeft, chunkSize;
	ssize_t bufBytes;
	char *dest = (char *) buf;

	// Copy loop.
	for(bytesLeft = len; bytesLeft > 0; bytesLeft -= chunkSize) {

		// Get count of bytes in file buffer and check for EOF or error.
		if((bufBytes = fileBufSize(pFastFile)) == 0)
			return len - bytesLeft;
		if(bufBytes < 0)
			return -1;

		// Store chunk in caller's buffer.
		chunkSize = bytesLeft > (size_t) bufBytes ? (size_t) bufBytes : bytesLeft;
		dest = (char *) memstpcpy((void *) dest, (void *) pFastFile->dataBufCur, chunkSize);
		pFastFile->dataBufCur += chunkSize;
		}

	return len;
	}

// Add a character to the line buffer.  Return status code.
static int lputc(short c, FastFile *pFastFile) {

	// Line buffer full?
	if(pFastFile->lineBufCur == pFastFile->lineBufEnd && growLineBuf(pFastFile) != 0)
		return -1;

	// Append character.
	*pFastFile->lineBufCur++ = c;
	return 0;
	}

// Read a delimited string from a data file into its line buffer, expanding the buffer as necessary to hold the entire line and
// delimiter(s), if any.  If the delimiter type is not defined (auto) and delimiter(s) are found, the delimiters in the file
// record are updated accordingly and used for subsequent calls.  Return line length, or zero if EOF.  If an error occurs,
// return -1.
ssize_t ffgets(FastFile *pFastFile) {
	short c;

	// At EOF (or error)?
	if((c = ffgetc(pFastFile)) < 0)
		return c + 1;

	// Reset line buffer and check line delimiters.
	pFastFile->lineBufCur = pFastFile->lineBuf;
	if(pFastFile->inpDelim1 == -1) {

		// Line delimiter(s) undefined.  Read bytes until a NL or CR found, or EOF reached.
		for(;;) {
			switch(c) {
				case -1:
					goto EOL;
				case -2:
					return -1;
				case '\n':
					pFastFile->inpDelim1 = '\n';
					pFastFile->inpDelim2 = -1;
					goto WrDelim;
				case '\r':
					pFastFile->inpDelim1 = '\r';
					switch(c = ffgetc(pFastFile)) {
						case -1:
							goto WrDelim;
						case -2:
							return -1;
						case '\n':
							pFastFile->inpDelim2 = '\n';
							goto WrDelim;
						}

					// Not a CR-LF sequence; "unget" the last character.
					--pFastFile->dataBufCur;
					goto WrDelim;
				}

			// No NL or CR found yet... onward.
			if(lputc(c, pFastFile) != 0)
				return -1;
			c = ffgetc(pFastFile);
			}
		}

	// We have line delimiter(s)... read a line.
	for(;;) {
		if(c == -1)	// Hit EOF, but at least one byte was read.
			goto EOL;
		if(c == -2)
			return -1;
		if(c == pFastFile->inpDelim1) {

			// Got match on first delimiter.  Need two?
			if(pFastFile->inpDelim2 == -1)

				// Nope, we're done.
				break;

			// Yes, check next character.
			if((c = ffgetc(pFastFile)) == -1) {

				// First of two delimiters was last character in file.  Treat it as ordinary.
				if(lputc(pFastFile->inpDelim1, pFastFile) != 0)
					return -1;
				goto EOL;
				}
			if(c == -2)
				return -1;
			if(c == pFastFile->inpDelim2)

				// Second delimiter matches also, we're done.
				break;

			// Not a match; "unget" the last character.
			--pFastFile->dataBufCur;
			c = pFastFile->inpDelim1;
			}

		// Delimiter not found, or two delimiters needed, but only one found.  Save the character and move onward.
		if(lputc(c, pFastFile) != 0)
			return -1;
		c = ffgetc(pFastFile);
		}
WrDelim:
	// Write delimiter(s) to line buffer.
	if(lputc(pFastFile->inpDelim1, pFastFile) != 0 ||
	 (pFastFile->inpDelim2 != -1 && lputc(pFastFile->inpDelim2, pFastFile) != 0))
		return -1;
EOL:
	// Got a (possibly zero-length) line.  Terminate it and return its length.
	return (lputc('\0', pFastFile) != 0) ? -1 : --pFastFile->lineBufCur - pFastFile->lineBuf;
	}

// Remove delimiter(s), if any, from an input line.  Return true if any were found; otherwise, false.
bool ffchomp(FastFile *pFastFile) {
	char *lineBufCur;
	bool result = false;

	// Delimiters set and non-null line?
	if(pFastFile->inpDelim1 != -1 && pFastFile->lineBufCur > pFastFile->lineBuf) {
		lineBufCur = pFastFile->lineBufCur - 1;		// Last character in line.
		if(pFastFile->inpDelim2 == -1) {

			// CR or NL delimiter.  Truncate line if delimiter matches.
			if(*lineBufCur == pFastFile->inpDelim1)
				goto Trunc;
			}
		else {
			// CR+LF delimiters.
			if(lineBufCur > pFastFile->lineBuf && *lineBufCur == pFastFile->inpDelim2 &&
			 *--lineBufCur == pFastFile->inpDelim1)
				goto Trunc;
			}
		}

	// Delimiters not found.
	goto Keep;
Trunc:
	*(pFastFile->lineBufCur = lineBufCur) = '\0';
	result = true;
Keep:
	return result;
	}

// Read entire data file into line buffer, expanding it as necessary, and append a null byte.  Return number of bytes read
// (zero if empty file) or -1 if error.  Routine assumes that no prior read operations have been performed on the file.
ssize_t ffslurp(FastFile *pFastFile) {
	ssize_t bufBytes;

	// Copy loop... read and save chunks in line buffer.
	pFastFile->lineBufCur = pFastFile->lineBuf;
	while((bufBytes = fileBufSize(pFastFile)) > 0) {

		// Expand line buffer for this chunk if needed.
		while(pFastFile->lineBufCur + bufBytes > pFastFile->lineBufEnd)
			if(growLineBuf(pFastFile) != 0)
				return -1;
		// Copy chunk.
		pFastFile->lineBufCur = (char *) memstpcpy((void *) pFastFile->lineBufCur, (void *) pFastFile->dataBufCur,
		 bufBytes);
		pFastFile->dataBufCur += bufBytes;
		}

	// Error or EOF.  Store null byte in line buffer and return length if the latter.
	if(bufBytes < 0 || lputc('\0', pFastFile) != 0)
		return -1;
	return --pFastFile->lineBufCur - pFastFile->lineBuf;
	}

// Write bytes to a data file.  Return status code.
int ffwrite(void *buf, size_t len, FastFile *pFastFile) {
	int rtnCode;
	size_t chunkSize, spaceLeft;
	char *src = (char *) buf;

	while(len > 0) {

		// Buffer full?
		if((spaceLeft = pFastFile->dataBufEnd - pFastFile->dataBufCur) == 0) {
			if((rtnCode = ffflush(pFastFile)) != 0)
				return rtnCode;
			spaceLeft = pFastFile->dataBufSize;
			}

		// Store chunk in buffer.
		chunkSize = len > spaceLeft ? spaceLeft : len;
		pFastFile->dataBufCur = (char *) memstpcpy((void *) pFastFile->dataBufCur, (void *) src, chunkSize);
		src += chunkSize;
		len -= chunkSize;
		}
	return 0;
	}

// Write a null-terminated string to a data file.  Return status code.
int ffputs(const char *str, FastFile *pFastFile) {

	return ffwrite((void *) str, strlen(str), pFastFile);
	}

// Write a character to a data file in visible form.  Pass "flags" to vizc().  Return status code.
int ffvizc(short c, ushort flags, FastFile *pFastFile) {
	char *str;

	return (str = vizc(c, flags)) == NULL || ffwrite((void *) str, strlen(str), pFastFile) != 0 ? -1 : 0;
	}

// Write bytes to a data file, exposing all invisible characters.  Pass "flags" to vizc().  If size is zero, assume mem is a
// null-terminated string; otherwise, write exactly size bytes.  Return status code.
int ffvizmem(const void *mem, size_t size, ushort flags, FastFile *pFastFile) {
	char *mem1 = (char *) mem;
	size_t n = (size == 0) ? strlen(mem1) : size;

	for(; n > 0; --n)
		if(ffvizc(*mem1++, flags, pFastFile) != 0)
			return -1;
	return 0;
	}

// Write a formatted string to a data file.  Return status code.
int ffprintf(FastFile *pFastFile, const char *fmt, ...) {
	int rtnCode;
	char *result;
	va_list varArgList;

	va_start(varArgList, fmt);
	rtnCode = vasprintf(&result, fmt, varArgList);
	va_end(varArgList);
	if(rtnCode < 0)
		return emsgsys(-1);
	rtnCode = ffputs(result, pFastFile);
	free((void *) result);
	return rtnCode;
	}
