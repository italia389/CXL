// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// lib.h		Header file for miscellaneous CXL library routines.

#ifndef lib_h
#define lib_h

// CXL version.
#define CXL_Version		"1.1.0"

#include <stdint.h>
#include "stdos.h"

// External function declarations.
extern int convDelim(const char *spec);
extern char *cxlvers(void);
extern int estrtol(const char *str, int base, long *pResult);
extern int estrtoul(const char *str, int base, ulong *pResult);
extern char *intf(long i);
extern uint prime(uint n);
extern uint32_t rand32(void);
extern uint32_t rand32Uniform(uint32_t upperBound);
extern char *uintf(ulong i);
#endif
