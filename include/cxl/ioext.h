// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// ioext.h		Header file for standard I/O extension routines.

#ifndef ioext_h
#define ioext_h

#include "stdos.h"
#include "string.h"
#include <stdio.h>

// Function declarations.
extern int fvizc(short c, ushort flags, FILE *pFile);
extern int fvizmem(const void *mem, size_t size, ushort flags, FILE *pFile);
#endif
