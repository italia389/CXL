// (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// strconv.c		Copy a string, converting escape sequences to single characters.

#include "stdos.h"
#include "cxl/excep.h"
#include "cxl/string.h"

// Copy src to dest, converting escape sequences to single characters and return status code.  The following escape sequences
// are recognized:
//	\t	011		Horizontal tab.
//	\n	012		Newline.
//	\v	013		Vertical tab.
//	\f	014		Form feed.
//	\r	015		Carriage return.
//	\e	033		Escape.
//	\s	040		Space.
//	\\	134		Backslash.
//	\nnn, \xnn, \Xnn	Ordinal value of a character in octal (1-3 digits) or hexadecimal (1-2 digits).  0nnn is
//				assumed to be octal and 0xnn, xnn, or Xnn is hexadecimal.
//	\?			Character ? itself, if not any of the above and not a null byte.
//
// Converted string is stored in dest, null-terminated.  The destination buffer is assumed to be at least the length of the
// source string + 1.  Source string is scanned until a null byte or termChar (which may also be a null byte) is found.  If
// pSrcEnd is not NULL, a pointer to this byte is stored in *pSrcEnd.
int strconv(char *dest, const char *src, const char **pSrcEnd, short termChar) {
	short c;

	// Loop through source string.
	while((c = *src) != '\0' && c != termChar) {

		// Process escaped characters.
		if(c == '\\') {
			if(*++src == '\0')
				goto BadSeq;

			// Initialize variables for \nn parsing, if any.  \0[xX]... and \[xX]... are hex, otherwise octal.
			short c2;
			int base = 8;
			int maxLen = 3;
			const char *src1;

			// Parse \[xX] and \0n... sequences.
			switch(*src++) {
				case 't':	// Tab
					c = '\t'; break;
				case 'r':	// CR
					c = '\r'; break;
				case 'n':	// NL
					c = '\n'; break;
				case 'e':	// Escape
					c = 033; break;
				case 's':	// Space
					c = 040; break;
				case 'f':	// Form feed
					c = '\f'; break;
				case 'v':	// Vertical tab
					c = '\v'; break;
				case 'x':
				case 'X':
					goto IsNum;
				case '0':
					if(*src == 'x') {
						++src;
IsNum:
						base = 16;
						maxLen = 2;
						goto GetNum;
						}
					// Fall through.
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					--src;
GetNum:
					// \nn found.  src is at first digit (if any).  Decode it.
					c = 0;
					src1 = src;
					while((c2 = *src) != '\0' && maxLen > 0) {
						if(c2 >= '0' && (c2 <= '7' || (c2 <= '9' && base != 8)))
							c = c * base + (c2 - '0');
						else {
							if(c2 >= 'A' && c2 <= 'Z')
								c2 += ('a' - 'A');
							if(base == 16 && (c2 >= 'a' && c2 <= 'f'))
								c = c * 16 + (c2 - ('a' - 10));
							else
								break;
							}

						// Char overflow?
						if(c > 0xff) {
BadSeq:
							return emsg(-1, "strconv(): Invalid \\nn sequence");
							}
						++src;
						--maxLen;
						}

					// Any digits decoded?
					if(src == src1)
						goto LitChar;

					// Valid sequence (null byte okay).
					break;
				default:	// Literal character.
LitChar:
					c = src[-1];
				}
			}

		// Not a backslash.  Vanilla character.
		else
			++src;

		// Save the character.
		*dest++ = c;
		}

	// Terminate converted string and return result.
	*dest = '\0';
	if(pSrcEnd != NULL)
		*pSrcEnd = src;
	return 0;
	}
