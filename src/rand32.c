// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// rand32.c		Routines for generating pseudo-random integers on any platform.

#include "stdos.h"
#include <stdint.h>
#include <stdlib.h>
#if !MACOS
#include <time.h>
#include <math.h>
#endif

// Return 32-bit unsigned pseudo-random integer.
uint32_t rand32(void) {
#if MACOS
	return arc4random();
#elif RAND_MAX < UINT32_MAX

	// The random() function is widely available on non-macOS (Linux) platforms; however, it returns numbers in range 0 to
	// 2^31, which is only half the range of a 32-bit integer.  To solve this problem, this routine maintains a "queue" of
	// random bits; that is, an unsigned 64-bit integer that is filled with random bits from random() calls as needed so
	// that a 32-bit integer can be extracted for each rand32() call.
	typedef struct {
		uint randSeed;
		uint64_t bitStream;
		short bits;
		} RandCtrl;
	static RandCtrl randCtrl = {0, 0, 0};
	uint32_t r;

	if(randCtrl.randSeed == 0)
		srandom(randCtrl.randSeed = time(NULL) & (uint) -1);

	// Call random() as needed to fill "bitStream" with random bits, 31 at a time.  When have 32 or more, return lower 32
	// bits and shift "bitStream" right.
	while(randCtrl.bits < 32) {
		randCtrl.bitStream |= ((uint64_t) random() << randCtrl.bits);
		randCtrl.bits += 31;
		}
	r = randCtrl.bitStream & 0xFFFFFFFF;
	randCtrl.bitStream >>= 32;
	randCtrl.bits -= 32;
	return r;
#else
	static uint randSeed = 0;

	if(randSeed == 0)
		srandom(randSeed = time(NULL) & (uint) -1);
	return (uint32_t) random();
#endif
	}

// Return 32-bit unsigned pseudo-random integer uniformly distributed across range 0 to upperBound - 1.
uint32_t rand32Uniform(uint32_t upperBound) {
#if MACOS
	return arc4random_uniform(upperBound);
#else
	return upperBound <= 1 ? 0 : (uint32_t) ((double) rand32() / (double) ((uint64_t) UINT32_MAX + 1) * upperBound);
#endif
	}
