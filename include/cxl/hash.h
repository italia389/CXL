// CXL (c) Copyright 2022 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// hash.h		Header file for hash routines.

#ifndef hash_h
#define hash_h

//#define HashDebug			stderr	// Debugging mode.
//#define HashDebug			logfile	// Debugging mode.

#include "cxl/datum.h"

typedef struct HashRec {
	struct HashRec *next;
	char *key;
	Datum *pValue;
	} HashRec;
typedef size_t HashSize;
typedef struct {
	HashRec **slots;		// Hash array.
	HashSize hashSize;		// Size of array -- number of slots (zero for default).
	size_t recCount;		// Current number of entries (nodes) in table.
	float loadFactor;		// Initial load factor to use when table is built or rebuilt (zero for default).
	float rebuildTrig;		// Minimum load factor which triggers a rebuild (zero for default).
	} HashTable;

#define hempty(hash)	((hash)->recCount == 0)

// External function declarations.
extern void hclear(HashTable *pHashTable);
extern int hcmp(const void *ppHashRec1, const void *ppHashRec2);
extern Datum *hdelete(HashTable *pHashTable, const char *key);
extern HashRec *heach(HashTable **pHashTable);
extern void hfree(HashTable *pHashTable);
extern int hinit(HashTable *pHashTable, HashSize hashSize, float loadFactor, float rebuildTrig);
extern HashTable *hnew(HashSize hashSize, float loadFactor, float rebuildTrig);
extern int hrename(HashTable *pHashTable, const char *oldKey, const char *newKey, int *pResult);
extern HashRec *hsearch(HashTable *pHashTable, const char *key);
extern HashRec *hset(HashTable *pHashTable, const char *key, Datum *pDatum, bool copy);
extern int hsort(const HashTable *pHashTable, int (*cmp)(const void *ppHashRec1, const void *ppHashRec2), HashRec ***pTable);
#ifdef HashDebug
extern void hstats(HashTable *pHashTable);
#endif
#endif
