// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// vizc.c		Routine for converting characters to visible form.

#include "stdos.h"
#include "cxl/excep.h"
#include "cxl/string.h"
#include <stdio.h>

// Return character c as a string, converting to visible form if non-text, as follows:
//	<NL>	012	Newline.
//	<CR>	015	Carriage return.
//	<ESC>	033	Escape.
//	<SPC>	040	Space (if VizSpace flag is set).
//	^X		Non-printable 7-bit character.
//	<NN>		Ordinal value of 8-bit character in hexadecimal (default) or octal.
//	c		Character c as a string, if none of the above.
//
// If an error occurs, an exception message is set and NULL is returned.
char *vizc(short c, ushort flags) {
	ushort base;
	static char literal[6];

	// Valid base?
	if((base = flags & VizBaseMask) > VizBaseMax) {
		(void) emsgf(-1, "vizc(): Invalid base (%hu)", base);
		return NULL;
		}

	// Expose character.
	switch(c) {
		case '\n':
			return "<NL>";
		case '\r':
			return "<CR>";
		case 033:
			return "<ESC>";
		default:
			if(c == ' ') {
				if(flags & VizSpace)
					return "<SPC>";
				goto DoChar;
				}
			if(c > ' ' && c <= '~') {
DoChar:
				literal[0] = c;
				literal[1] = '\0';
				break;
				}
			if(c < 0 || c > 0xFF)
				return "<?>";
			else {
				char *fmt;
				if(c <= 0x7F) {
					fmt = "^%c";
					c ^= 0x40;
					}
				else
					fmt = (base == VizBaseOctal) ? "<%.3o>" : (base == VizBaseHex) ? "<%.2x>" : "<%.2X>";
				sprintf(literal, fmt, c);
				}
		}

	return literal;
	}
