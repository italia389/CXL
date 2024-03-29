// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// getSwitch.h		Header file for getSwitch() routine.

#ifndef getSwitch_h
#define getSwitch_h

#include "stdos.h"

// Switch objects.
typedef struct {
	const char **names;			// Pointer to NULL-terminated array of switch names.
	ushort flags;				// Descriptor flags.
	} Switch;
typedef struct {
	const char *name;			// Pointer to actual switch name found.
	char *arg;				// Pointer to switch argument if found, otherwise NULL.
	} SwitchResult;

// Switch descriptor flags.
#define SF_NumericSwitch	0x0001		// Numeric switch (does not accept an argument), otherwise named.
#define SF_RequiredSwitch	0x0002		// Required switch, otherwise optional.
#define SF_PlusType		0x0004		// Numeric switch is +n type, otherwise -n.
#define SF_NoArg		(1 << 2)	// Switch does not accept an argument.
#define SF_OptionalArg		(2 << 2)	// Switch may accept an argument.
#define SF_RequiredArg		(3 << 2)	// Switch requires an argument.
#define SF_ArgMask		(0x3 << 2)	// Mask for argument type.

#define SF_NumericArg		0x0010		// Switch argument must be a number.
#define SF_AllowSign		0x0020		// Numeric literal argument may have a leading sign (+ or -).
#define SF_AllowDecimal		0x0040		// Numeric literal argument may contain a decimal point.
#define SF_AllowRepeat		0x0080		// Switch may be repeated.
#define SF_AllowNullArg		0x0100		// Switch argument may be null.

#define NSMinusKey		"-zzz-"		// Dummy hash key to use for - numeric switch.
#define NSPlusKey		"+zzz+"		// Dummy hash key to use for + numeric switch.

// External function declarations.
extern int getSwitch(int *pArgCount, char ***pArgList, Switch **pSwitchTable, uint switchCount,
 SwitchResult *pResult);
#endif
