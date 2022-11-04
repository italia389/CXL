// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// bmsearch.c		Boyer-Moore search routines.

#include "stdos.h"
#include "cxl/excep.h"
#include "cxl/bmsearch.h"
#include "cxl/string.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define max(a, b)	((a < b) ? b : a)

// Build "bad character" table.  Pattern length is assumed to be > 0.
//
// badChar[c] contains the distance between the last character of pat and the rightmost occurrence of c in pat.  If c does not
// occur in pat, then badChar[c] == patLen.  If c is at string[i] and c != pat[patLen - 1], we can safely shift i over by
// badChar[c], which is the minimum distance needed to shift pat forward to get string[i] lined up with some character in pat.
// This algorithm runs in CharSetLen + patLen time.
static void makeBadCharTable(BMPat *pPat) {
	short c;
	CharOffset i, *pTab, *pTabEnd;
	CharOffset *badChar = pPat->data.badChar;
	CharOffset patLen1 = pPat->len - 1;

	// First, set all characters to maximum jump distance.
	pTabEnd = (pTab = badChar) + CharSetLen;
	do {
		*pTab++ = pPat->len;
		} while(pTab < pTabEnd);

	// Now set jump distance for each character in the pattern.  Set both cases of letters if ignoring case.
	for(i = 0; i < patLen1; ++i) {
		c = pPat->pat[i];
		badChar[c] = patLen1 - i;
		if((pPat->cflags & PatIgnoreCase) && isalpha(c))
			badChar[c + (c >= 'a' ? -32 : 32)] = patLen1 - i;
		}
	}

// Build "good suffix" table.  Pattern length is assumed to be > 0.
static void makeGoodSufTable(BMPat *pPat) {
	CharOffset patLen = pPat->len, patLen1 = patLen - 1;
	CharOffset f, g, i, j, *pTab, *pTabEnd;
	CharOffset *suff = pPat->data.suff;
	CharOffset *goodSuf = pPat->data.goodSuf;
	char *pat = pPat->pat;

	// First, build suff table.
	suff[patLen1] = patLen;
	g = patLen1;
	for(i = patLen - 2; i >= 0; --i) {
		if(i > g && suff[i + patLen1 - f] < i - g)
			suff[i] = suff[i + patLen1 - f];
		else {
			if(i < g)
				g = i;
			f = i;
			while(g >= 0 && (pat[g] == pat[g + patLen1 - f] ||
			 ((pPat->cflags & PatIgnoreCase) && tolower(pat[g]) == tolower(pat[g + patLen1 - f]))))
				--g;
			suff[i] = f - g;
			}
		}

	// Next, set all pattern positions to maximum jump distance.
	pTabEnd = (pTab = goodSuf) + patLen;
	do {
		*pTab++ = patLen;
		} while(pTab < pTabEnd);

	// Now set jump distances according to suffixes found.
	j = 0;
	for(i = patLen1; i >= 0; --i)
		if(suff[i] == i + 1)
			for(; j < patLen1 - i; ++j)
				if(goodSuf[j] == patLen)
					goodSuf[j] = patLen1 - i;
	for(i = 0; i <= patLen - 2; ++i)
		goodSuf[patLen1 - suff[i]] = patLen1 - i;
	}

// Compile given fixed-length pattern and return status code.
int bmncomp(BMPat *pPat, const char *pat, CharOffset len, unsigned cflags) {

	// Pattern cannot be empty.
	if(len == 0)
		return emsg(-1, "Search pattern cannot be empty");

	// Get space for patterns and jump tables.
	if((pPat->pat = (char *) malloc(len + 1)) == NULL ||
	 (pPat->data.badChar = (CharOffset *) malloc(sizeof(CharOffset) * (CharSetLen + len * 2) +
	  (cflags & PatIgnoreCase ? len : 0))) == NULL) {
		cxlExcep.flags |= ExcepMem;
		return emsgsys(-1);
		}
	pPat->data.goodSuf = pPat->data.badChar + CharSetLen;
	pPat->data.suff = pPat->data.goodSuf + len;

	// Save arguments.
	pPat->cflags = cflags;
	pPat->len = len;
	*((char *) memstpcpy(pPat->pat, pat, len)) = '\0';

	// Reverse pattern in place if PatReverse specified.
	if((cflags & PatReverse) && len > 1)
		memrev(pPat->pat, len);

	// Create lower-case pattern if PatIgnoreCase specified.
	if(cflags & PatIgnoreCase) {
		char *lcPat;
		const char *pat1, *patEnd;

		lcPat = pPat->data.lcPat = (char *) (pPat->data.suff + len);
		patEnd = (pat1 = pPat->pat) + len;
		do {
			*lcPat++ = tolower(*pat1++);
			} while(pat1 < patEnd);
		}
	else
		pPat->data.lcPat = NULL;

	// Make jump tables.
	makeBadCharTable(pPat);
	makeGoodSufTable(pPat);

	return 0;
	}

// Compile given null-terminated pattern and return status code.
int bmcomp(BMPat *pPat, const char *pat, unsigned cflags) {

	return bmncomp(pPat, pat, strlen(pat), cflags);
	}

// Free given compiled pattern.
void bmfree(BMPat *pPat) {

	free((void *) pPat->data.badChar);
	free((void *) pPat->pat);
	}

// Find first occurrence of pattern in given fixed-length string, scanning forward or backward.  Return pointer to beginning of
// match if found, otherwise NULL.  Uses Apostolico-Giancarlo search algorithm, which is a refinement of the Boyer-Moore
// algorithm.
const char *bmnexec(const BMPat *pPat, const char *string, size_t len) {
	CharOffset i, k, s, shift;
	const char *str, *strLimit, *result = NULL;
	char *pat = pPat->cflags & PatIgnoreCase ? pPat->data.lcPat : pPat->pat;
	CharOffset patLen = pPat->len;
	CharOffset patLen1 = (CharOffset) patLen - 1;
	CharOffset *badChar = pPat->data.badChar;
	CharOffset *goodSuf = pPat->data.goodSuf;
	CharOffset *suff = pPat->data.suff;
	CharOffset skip[patLen];

	memset(skip, 0, patLen * sizeof(CharOffset));
	if(pPat->cflags & PatReverse) {

		// Perform backward search.
		str = string + len - 1;
		strLimit = string + patLen1;
		while(str >= strLimit) {
			i = patLen1;
			while(i >= 0) {
				k = skip[i];
				s = suff[i];
				if(k > 0)
					if(k > s) {
						if(i + 1 == s)
							goto BackMatch;
						i -= s;
						break;
						}
					else {
						i -= k;
						if(k < s) {
							break;
							}
						}
				else {
					if(pat[i] == (pPat->cflags & PatIgnoreCase ? tolower(str[-i]) : str[-i]))
						--i;
					else
						goto BackMismatch;
					}
				}

			if(i < 0) {
BackMatch:
				result = str - patLen1;
				break;
				}
BackMismatch:
			// Match not found.  "Jump ahead" for next attempt.
			skip[patLen1] = patLen1 - i;
			shift = max(badChar[(int) str[-i]] - patLen1 + i, goodSuf[i]);
			str -= shift;
			memcpy(skip, skip + shift, (patLen - shift) * sizeof(CharOffset));
			memset(skip + patLen - shift, 0, shift * sizeof(CharOffset));
			}
		}
	else {
		// Perform forward search.
		strLimit = (str = string) + len - patLen;
		while(str <= strLimit) {
			i = patLen1;
			while(i >= 0) {
				k = skip[i];
				s = suff[i];
				if(k > 0)
					if(k > s) {
						if(i + 1 == s)
							goto ForwMatch;
						i -= s;
						break;
						}
					else {
						i -= k;
						if(k < s) {
							break;
							}
						}
				else {
					if(pat[i] == (pPat->cflags & PatIgnoreCase ? tolower(str[i]) : str[i]))
						--i;
					else
						goto ForwMismatch;
					}
				}

			if(i < 0) {
ForwMatch:
				result = str;
				break;
				}
ForwMismatch:
			// Match not found.  "Jump ahead" for next attempt.
			skip[patLen1] = patLen1 - i;
			shift = max(badChar[(int) str[i]] - patLen1 + i, goodSuf[i]);
			str += shift;
			memcpy(skip, skip + shift, (patLen - shift) * sizeof(CharOffset));
			memset(skip + patLen - shift, 0, shift * sizeof(CharOffset));
			}
		}

	return result;
	}

// Find first occurrence of pattern in given null-terminated string.  Return pointer to beginning of match if found,
// otherwise NULL.
const char *bmexec(const BMPat *pPat, const char *string) {

	return bmnexec(pPat, string, strlen(string));
	}

// Find first occurrence of pattern in user data, scanning forward or backward.  Return offset to beginning of match if found,
// otherwise -1.  Uses Apostolico-Giancarlo search algorithm.
CharOffset bmuexec(const BMPat *pPat, bool (*nextChar)(short *pc, void *context), void *context) {
	short c;
	int count;
	CharOffset i, k, s, shift, offset;
	char *pat = pPat->cflags & PatIgnoreCase ? pPat->data.lcPat : pPat->pat;
	CharOffset patLen = pPat->len;
	CharOffset patLen1 = (CharOffset) patLen - 1;
	CharOffset *badChar = pPat->data.badChar;
	CharOffset *goodSuf = pPat->data.goodSuf;
	CharOffset *suff = pPat->data.suff;
	CharOffset skip[patLen];
	char *str, strWind[patLen];

	// Initialize.
	memset(skip, 0, patLen * sizeof(CharOffset));
	offset = 0;

	// Fill string window.
	str = strWind;
	count = patLen;
	do {
		if(nextChar(&c, context))
			goto NoMatch;
		*str++ = c;
		} while(--count > 0);

	// Perform search.
	for(;;) {
		i = patLen1;
		while(i >= 0) {
			k = skip[i];
			s = suff[i];
			if(k > 0)
				if(k > s) {
					if(i + 1 == s)
						goto Match;
					i -= s;
					break;
					}
				else {
					i -= k;
					if(k < s) {
						break;
						}
					}
			else {
				if(pat[i] == (pPat->cflags & PatIgnoreCase ? tolower(strWind[i]) : strWind[i]))
					--i;
				else
					goto Mismatch;
				}
			}

		if(i < 0)
			break;
Mismatch:
		// Match not found.  "Jump ahead" for next attempt.
		skip[patLen1] = patLen1 - i;
		shift = max(badChar[(int) strWind[i]] - patLen1 + i, goodSuf[i]);
		offset += shift;

		// Shift string window left by "shift" characters and refill it.
		memcpy(strWind, strWind + shift, patLen - shift);
		str = strWind + patLen - shift;
		count = shift;
		do {
			if(nextChar(&c, context))
				goto NoMatch;
			*str++ = c;
			} while(--count > 0);

		// Shift skip array left by "shift" characters and set remainder to zero.
		memcpy(skip, skip + shift, (patLen - shift) * sizeof(CharOffset));
		memset(skip + patLen - shift, 0, shift * sizeof(CharOffset));
		}
Match:
	return offset;
NoMatch:
	return -1;
	}
