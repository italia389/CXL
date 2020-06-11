// (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// hash.c		Hash table routines.

#include "stdos.h"
#include "cxl/excep.h"
#include "cxl/lib.h"
#include "cxl/hash.h"
#ifdef HashDebug
#include <stdio.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

// Hash table parameters.  "Load factor" is n/k where "n" is number of entries, and "k" is number of slots (hash size).  The
// hnew() function mandates that the initial load factor be <= 1.0 and the rebuild trigger be greater than the initial load
// factor, to avoid a table rebuild after every insert.
#define DefaultHashSize		67	// Starting hash size if not specified by caller.
#define InitialLoadFactor	0.5	// Initial load factor if not specified by caller (when table built or rebuilt).
#define MaxLoadFactor		1.0	// Maximum allowed value of initial load factor.
#define DefaultRebuildTrigger	1.65	// Default minimum load factor which triggers a rebuild.

#ifdef HashDebug
extern FILE *HashDebug;
#endif

// Hash a key and return integer result.
static HashSize hash(const char *key, HashSize hashSize) {
	size_t k = 0;
	while(*key != '\0')
		k = (k << 2) + *key++;
	return k % hashSize;
	}

// Search for (string) key in hash table.  Return pointer to HashRec if found; otherwise, NULL.  If pTableSlot is not NULL, set
// it to hash table entry (slot).
static HashRec *hsrch(Hash *hashTable, const char *key, HashRec ***pTableSlot) {
	HashRec *pHashRec;
	HashRec **tableSlot = hashTable->table + hash(key, hashTable->hashSize);

	if(pTableSlot != NULL)
		*pTableSlot = tableSlot;

	for(pHashRec = *tableSlot; pHashRec != NULL; pHashRec = pHashRec->next) {
		if(strcmp(pHashRec->key, key) == 0)
			return pHashRec;
		}

	return NULL;
	}

// Walk through a hash table returning each hash record in sequence, or NULL if none left.  "pHashTable" is an indirect pointer
// to the Hash object and is modified by this routine.
HashRec *heach(Hash **pHashTable) {
	HashRec *pHashRec;
	static struct {
		HashRec **ppHashRec, **ppHashRecEnd, *pHashRec;
		} hashState = {
			NULL, NULL, NULL
			};

	// Validate and initialize control pointers.
	if(*pHashTable != NULL) {
		if((*pHashTable)->recCount == 0)
			goto Done;
		hashState.ppHashRecEnd = (hashState.ppHashRec = (*pHashTable)->table) + (*pHashTable)->hashSize;
		hashState.pHashRec = *hashState.ppHashRec;
		*pHashTable = NULL;
		}
	else if(hashState.ppHashRec == NULL)
		return NULL;

	// Get next record and return it, or NULL if none left.
	while(hashState.pHashRec == NULL) {
		if(++hashState.ppHashRec == hashState.ppHashRecEnd) {
Done:
			hashState.ppHashRec = NULL;
			return NULL;
			}
		hashState.pHashRec = *hashState.ppHashRec;
		}

	pHashRec = hashState.pHashRec;
	hashState.pHashRec = pHashRec->next;
	return pHashRec;
	}

// Create or rebuild given hash table.  Return status code.
static int hbuild(Hash *hashTable) {
	HashSize newHashSize;
	HashRec **newTable;

	// Determine new hash size.
	if(hashTable->recCount == 0) {
		if(hashTable->hashSize == 0)
			newHashSize = DefaultHashSize;
		else if((newHashSize = prime(hashTable->hashSize)) == 0)
			newHashSize = hashTable->hashSize;
		}
	else if((newHashSize = prime((uint) round(hashTable->recCount / hashTable->loadFactor))) == 0)
		return emsgf(-1, "Cannot resize hash table for %lu entries", hashTable->recCount);

	// Allocate array of NULL hash pointers.
	if((newTable = (HashRec **) calloc(newHashSize, sizeof(HashRec *))) == NULL) {
		cxlExcep.flags |= ExcepMem;
		return emsgsys(-1);
		}

	// If hash table not empty, rebuild it.
	if(hashTable->recCount > 0) {
		HashRec **tableSlot;
		Hash *hashTable1 = hashTable;
		HashRec *pHashRec = heach(&hashTable1);
#ifdef HashDebug
		fprintf(HashDebug, "Rebuilding hash at recCount %lu, hashSize %lu -> %lu...\n",
		 hashTable->recCount, hashTable->hashSize, newHashSize);
#endif
		do {
			// Get slot and add record to front of linked list.
			tableSlot = newTable + hash(pHashRec->key, newHashSize);
			pHashRec->next = *tableSlot;
			*tableSlot = pHashRec;
			} while((pHashRec = heach(&hashTable1)) != NULL);

		// Release old hash space.
		free((void *) hashTable->table);
#ifdef HashDebug
		fputs("rebuild completed.\n", HashDebug);
#endif
		}

	hashTable->hashSize = newHashSize;
	hashTable->table = newTable;
	return 0;
	}

// Create hash table and return pointer to it, or NULL if error.  hashSize is the initial size of the table, loadFactor is used
// to calculate the new hash size when the table is built or rebuilt, and rebuildTrig is the minimum load factor which triggers
// a rebuild.  All three parameters are saved in the Hash object.  If hashSize, loadFactor, and/or rebuildTrig is zero, a
// default value is used.
Hash *hnew(HashSize hashSize, float loadFactor, float rebuildTrig) {
	Hash *hashTable;

	// Create hash record.
	if((hashTable = (Hash *) malloc(sizeof(Hash))) == NULL) {
		cxlExcep.flags |= ExcepMem;
		emsgsys(-1);
		}
	else {
		// Set hash table parameters and do sanity checks.
		hashTable->loadFactor = (loadFactor == 0.0) ? InitialLoadFactor : loadFactor;
		hashTable->rebuildTrig = (rebuildTrig == 0.0) ? DefaultRebuildTrigger : rebuildTrig;
		if(hashTable->loadFactor < 0.0)
			emsgf(-1, "Initial hash table load factor %.2f cannot be less than zero", hashTable->loadFactor);
		else if(hashTable->rebuildTrig < 0.0)
			emsgf(-1, "Hash table rebuild trigger %.2f cannot be less than zero", hashTable->rebuildTrig);
		else if(hashTable->loadFactor > MaxLoadFactor)
			emsgf(-1, "Initial hash table load factor %.2f cannot be greater than %.2f", hashTable->loadFactor,
			 MaxLoadFactor);
		else if(hashTable->rebuildTrig <= hashTable->loadFactor)
			emsgf(-1, "Hash table rebuild trigger %.2f must be greater than initial load factor %.2f",
			 hashTable->rebuildTrig, hashTable->loadFactor);
		else {
			hashTable->hashSize = hashSize;
			hashTable->recCount = 0;

			// Let hbuild() do the rest.
			if(hbuild(hashTable) == 0)
				return hashTable;
			}
		}

	// Error detected.
	return NULL;
	}

// Save key in given hash record (on heap) and add record to given hash table.  Return status code.
static int hsave(Hash *hashTable, HashRec **tableSlot, HashRec *pHashRec, const char *key) {

	// Save key.
	char *str = (char *) malloc(strlen(key) + 1);
	if(str == NULL) {
		cxlExcep.flags |= ExcepMem;
		return emsgsys(-1);
		}
	pHashRec->key = strcpy(str, key);

	// Add record to front of linked list.
	pHashRec->next = *tableSlot;
	*tableSlot = pHashRec;
	++hashTable->recCount;

	return 0;
	}

// Store a Datum object (or a copy if "copy" is true) in given hash table, given key.  Return pointer to its hash record, or
// NULL if error.  If pDatum is NULL, create and store a nil Datum object.
HashRec *hset(Hash *hashTable, const char *key, Datum *pDatum, bool copy) {
	HashRec *pHashRec, **tableSlot;
	bool newEntry = false;

	// Value given?
	if(pDatum == NULL)
		copy = true;

	// Does key exist?
	if((pHashRec = hsrch(hashTable, key, &tableSlot)) == NULL) {

		// Nope, create new record.
		if((pHashRec = (HashRec *) malloc(sizeof(HashRec))) == NULL) {
			cxlExcep.flags |= ExcepMem;
			emsgsys(-1);
			return NULL;
			}

		// Add record to table.
		if((copy && dnew(&pHashRec->pValue) != 0) || hsave(hashTable, tableSlot, pHashRec, key) != 0)
			return NULL;
		newEntry = true;
		}

	// Yes, clear existing entry.
	else if(copy)
		dclear(pHashRec->pValue);
	else
		ddelete(pHashRec->pValue);

	// Save value if given.
	if(pDatum != NULL) {
		if(!copy)
			pHashRec->pValue = pDatum;
		else if(datcpy(pHashRec->pValue, pDatum) != 0)
			return NULL;
		}

	// Return result.  If hash table is too big, rebuild it.
	return (newEntry && (double) hashTable->recCount / hashTable->hashSize >= hashTable->rebuildTrig &&
	 hbuild(hashTable) != 0) ? NULL : pHashRec;
	}

// Compare keys of two hash records and return result -- helper function for qsort().
int hcmp(const void *ppHashRec1, const void *ppHashRec2) {

	return strcmp((*(HashRec **) ppHashRec1)->key, (*(HashRec **) ppHashRec2)->key);
	}

// Remove hash entry, given key.  If entry does not exist, return NULL; otherwise, delete key, remove entry from table, and
// return hash record (which will have invalid "key" and "next" members).
static HashRec *hremove(Hash *hashTable, const char *key) {
	HashRec *pHashRec0, *pHashRec1;
	HashRec **tableSlot = hashTable->table + hash(key, hashTable->hashSize);

	pHashRec0 = NULL;
	for(pHashRec1 = *tableSlot; pHashRec1 != NULL; pHashRec1 = pHashRec1->next) {
		if(strcmp(pHashRec1->key, key) == 0) {

			// Found entry.  Remove it from linked list.
			if(pHashRec0 == NULL)
				*tableSlot = pHashRec1->next;
			else
				pHashRec0->next = pHashRec1->next;

			// Clear key and return record.
			free((void *) pHashRec1->key);
			--hashTable->recCount;
			return pHashRec1;
			}
		pHashRec0 = pHashRec1;
		}

	// Not found.
	return NULL;
	}

// Delete hash entry, given key, and return its Datum object or NULL if entry does not exist.
Datum *hdelete(Hash *hashTable, const char *key) {
	HashRec *pHashRec = hremove(hashTable, key);
	if(pHashRec == NULL)
		return NULL;
	Datum *pDatum = pHashRec->pValue;
	free((void *) pHashRec);
	return pDatum;
	}

// Rename hash entry, given old and new keys.  If pResult not NULL, set *pResult to zero if entry was renamed; otherwise, set to
// -1 if old key does not exist or 1 if new key already exists.  Return status code.
int hrename(Hash *hashTable, const char *oldKey, const char *newKey, int *pResult) {
	HashRec *pHashRec, **tableSlot;
	int result;
	int rtnCode = 0;

	// Does new key already exist?
	if(hsrch(hashTable, newKey, &tableSlot) != NULL)
		result = 1;

	// Delete old key if it exists.
	else if((pHashRec = hremove(hashTable, oldKey)) == NULL)
		result = -1;

	// Old key was found (and deleted).  Add new key.
	else {
		result = 0;
		rtnCode = hsave(hashTable, tableSlot, pHashRec, newKey);
		}

	if(pResult != NULL)
		*pResult = result;
	return rtnCode;
	}

// Clear given hash table; that is, delete all nodes, leaving an empty array and the Hash object itself.
void hclear(Hash *hashTable) {
	HashRec *pHashRec0, *pHashRec1, **ppHashRec, **ppHashRecEnd;

	ppHashRecEnd = (ppHashRec = hashTable->table) + hashTable->hashSize;
	do {
		if(*ppHashRec != NULL) {
			pHashRec0 = *ppHashRec;
			do {
				pHashRec1 = pHashRec0->next;
				ddelete(pHashRec0->pValue);
				free((void *) pHashRec0->key);
				free((void *) pHashRec0);
				} while((pHashRec0 = pHashRec1) != NULL);
			*ppHashRec = NULL;
			}
		} while(++ppHashRec < ppHashRecEnd);
	hashTable->recCount = 0;
	}

// Free given hash table.
void hfree(Hash *hashTable) {

	hclear(hashTable);
	free((void *) hashTable->table);
	free((void *) hashTable);
	}

// Search for key in hash table.  Return pointer to HashRec if found; otherwise, NULL.
HashRec *hsearch(Hash *hashTable, const char *key) {

	return hsrch(hashTable, key, NULL);
	}

// Sort given hash and return result as an array of record pointers in *pTable.  "cmp" is the entry comparison routine to pass
// to qsort().  If *pTable is not NULL, it is assumed to be a pointer to an array of hashTable->recCount elements, which will be
// used for the result.  Otherwise, an array will be allocated on the heap (if the hash is not empty) and returned in *pTable.
// The pointer to the array should be passed to free() to release the allocated storage when it is no longer needed.  In either
// case, if the hash is empty, *pTable is set to NULL.  Return status code.
int hsort(Hash *hashTable, int (*cmp)(const void *pHashRec1, const void *pHashRec2), HashRec ***pTable) {

	if(hashTable->recCount == 0)
		*pTable = NULL;
	else {
		Hash *hashTable1;
		HashRec **ppDestRec, **ppDestRec0, *pSrcRec;

		// Allocate array if needed and copy pointers.
		if(*pTable != NULL)
			ppDestRec0 = *pTable;
		else if((ppDestRec0 = (HashRec **) malloc(sizeof(HashRec *) * hashTable->recCount)) == NULL) {
			cxlExcep.flags |= ExcepMem;
			return emsgsys(-1);
			}
		hashTable1 = hashTable;
		ppDestRec = ppDestRec0;
		while((pSrcRec = heach(&hashTable1)) != NULL)
			*ppDestRec++ = pSrcRec;

		// Sort array and return result.
		qsort((void *) ppDestRec0, hashTable->recCount, sizeof(*ppDestRec0), cmp);
		*pTable = ppDestRec0;
		}

	return 0;
	}

#ifdef HashDebug
void hstats(Hash *hashTable) {

	fprintf(HashDebug,
	 "\n### HASH STATISTICS\n%20s %lu\n%20s %lu\n%20s %.2f\n%20s %.2f\n", "Size:", hashTable->hashSize, "Entries:",
	 hashTable->recCount, "Initial load factor:", hashTable->loadFactor, "Rebuild trigger:", hashTable->rebuildTrig);
	if(hashTable->recCount > 0) {
		HashRec *pHashRec, **ppHashRec = hashTable->table, **ppHashRecEnd = ppHashRec + hashTable->hashSize;
		size_t count, maxSlot = 0, slotCounts[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		do {
			count = 0;
			for(pHashRec = *ppHashRec++; pHashRec != NULL; pHashRec = pHashRec->next)
				++count;
			if(count > maxSlot)
				maxSlot = count;
			if(count > 9)
				count = 9;
			++slotCounts[count];
			} while(ppHashRec < ppHashRecEnd);
		fprintf(HashDebug, "%20s %lu\n%20s %.2f\n%20s\n", "Max slot size:", maxSlot, "Current load factor:",
		 (float) hashTable->recCount / hashTable->hashSize, "Slot counts:");
		count = 0;
		do {
			fprintf(HashDebug, "%19lu: %lu\n", count, slotCounts[count]);
			} while(++count < 10);
		}
	}
#endif
