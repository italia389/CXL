// CXL (c) Copyright 2020 Richard W. Marinelli
//
// This work is licensed under the GNU General Public License (GPLv3).  To view a copy of this license, see the
// "License.txt" file included with this distribution or visit http://www.gnu.org/licenses/gpl-3.0.en.html.
//
// getSwitch.c		Routine for processing command-line switches.

#include "stdos.h"
#include "cxl/excep.h"
#include "cxl/lib.h"
#include "cxl/string.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "cxl/getSwitch.h"
#include "cxl/hash.h"

// Local declarations.
typedef struct {
	Switch *pSwitch;			// Pointer to switch descriptor in switch table.
	uint foundCount;			// Number of occurrences found.
	} SwitchState;

#define DupHashSize		19		// Initial size of hash table used for duplicate-switch checking.
#define ssPtr(pHashRec)		((SwitchState *) pHashRec->pValue->u.blob.mem)
//#define GSDebugFile		stderr
#ifdef GSDebugFile
extern FILE *GSDebugFile;
#endif

// Get switch of form -sw1 val1 -sw2 -n +n...
//
// Arguments:
//	pArgCount	Pointer to count of string arguments in *pArgList array (typically &argCount from main() function).
//	pArgList	Indirect pointer to array of string arguments (typically &argList from main() function).
//	pSwitchTable	Indirect pointer to switch table (an array of Switch descriptors), which contains a descriptor for
//			each possible switch.
//	switchCount	Number of switches in switch table.
//	pResult		Pointer to result record, containing primary switch name and its value, if any.
//
// Notes:
//  1. Each invocation of this routine returns one switch.  Routine sets *pSwitchTable to NULL after first call to process
//     subsequent switches.
//  2. If a switch is found, values in *pResult are set as appropriate and a 1-relative index of switch in the switch table is
//     returned.  If a numeric switch is found, "name" is set to NULL and "value" is set to the switch that was found, including
//     the leading '-' or '+' character.
//  3. All switch names in the switch table must be unique -- duplicates are not allowed.  Also, a maximum of one -n switch
//     descriptor (SF_NumericSwitch flag set and SF_PlusType not set) and one +n switch descriptor (both SF_NumericSwitch and
//     SF_PlusType flags set) may be specified.
//  4. If any of the following occurs, switch scanning stops, *pArgCount is set to the number of arguments remaining, *pArgList
//     is set to the argument following the last switch found, and zero is returned:
//		a. *pArgCount arguments have been processed;
//		b. **pArgList is a NULL pointer;
//		c. a "--" switch or "-" argument is found;
//		d. a string is found which does not begin with '-' (or '+' if a switch descriptor of type SF_PlusType exists
//		   in the switch table).
//  5. If a switch requires an argument and an argument is found which begins with '--' or '++', the string following the first
//     '-' or '+' is returned as the argument.
//  6. If an error occurs, an exception message is set and -1 is returned.
int getSwitch(int *pArgCount, char ***pArgList, Switch **pSwitchTable, uint switchCount, SwitchResult *pResult) {
	static char myName[] = "getSwitch";
	const char *tableSwitch, **pTableSwitch;
	char *arg, *argSwitch = NULL, *str0, *str1;
	int switchIndex = 0;
	ushort argType;
	enum {expectOptionalArg, expectRequiredArg, expectSwitch} stateExpected = expectSwitch;
	enum {foundArg, foundNoArg, foundNullArg, foundSwitch} stateFound;
	Switch *pSwitch, *pSwitchEnd;
	char *digits = "0123456789";
	Hash *hashTable;
	HashRec *pHashRec;
	SwitchState *pState;
	static struct {
		Switch *pSwitch;		// Pointer to first descriptor in switch table.
		Hash *hashTable;		// Hash of switches expected and found (for duplicate checking).
		} state;


#ifdef GSDebugFile
	fprintf(GSDebugFile, "%s(): pArgCount %.8X (%d), pArgList %.8X, *pArgList %.8X (%s)...\n", myName,
	 (uint) pArgCount, *pArgCount, (uint) pArgList, (uint) *pArgList, **pArgList);
#endif
	// Intialize variables and validate switch table if first call.
	if(*pSwitchTable != NULL) {
#ifdef GSDebugFile
		fputs("getSwitch(): Initializing...\n", GSDebugFile);
#endif
		if((state.hashTable = hnew(DupHashSize, 0.0, 0.0)) == NULL)
			return -1;
		for(pSwitchEnd = (state.pSwitch = pSwitch = *pSwitchTable) + switchCount; pSwitch < pSwitchEnd; ++pSwitch) {

			// Create one SwitchState object that the primary switch and all its aliases will point to.
			if((pState = (SwitchState *) malloc(sizeof(SwitchState))) == NULL) {
				cxlExcep.flags |= ExcepMem;
				return emsgsys(-1);
				}
			pState->pSwitch = pSwitch;
			pState->foundCount = 0;

			// Check switch type.
			if(pSwitch->flags & SF_NumericSwitch) {

				// Numeric switch.  Check if duplicate.
				tableSwitch = (pSwitch->flags & SF_PlusType) ? NSPlusKey : NSMinusKey;
				if(hsearch(state.hashTable, tableSwitch) != NULL)
					return emsgf(-1, "%s(): Multiple numeric (%c) switch descriptors found", myName,
					 (pSwitch->flags & SF_PlusType) ? '+' : '-');
				if((pHashRec = hset(state.hashTable, tableSwitch, NULL, false)) == NULL)
					return -1;
				dsetblobref((void *) pState, 0, pHashRec->pValue);
				}
			else {
				// Standard switch.  Check if argument type specified.
				if((pSwitch->flags & SF_ArgMask) == 0)
					return emsgf(-1, "%s(): Argument type not specified for -%s switch", myName,
					 *pSwitch->names);

				// Check switch name plus any aliases for duplicates.
				for(pTableSwitch = pSwitch->names; (tableSwitch = *pTableSwitch) != NULL; ++pTableSwitch) {
					if(hsearch(state.hashTable, tableSwitch) != NULL)
						return emsgf(-1, "%s(): Multiple -%s switch descriptors found",
						 myName, tableSwitch);
					if((pHashRec = hset(state.hashTable, tableSwitch, NULL, false)) == NULL)
						return -1;
					dsetblobref((void *) pState, 0, pHashRec->pValue);
					}
				}
			}

		// Switch table is valid.  Clear table pointer.
		*pSwitchTable = NULL;
		}
	else if(state.pSwitch == NULL)
		return emsgf(-1, "%s(): Switch table not specified", myName);

	// Loop through argument list and parse a switch.
	for(;;) {
		// Set 'found' state.
		if(*pArgCount == 0 || (arg = **pArgList) == NULL) {
			stateFound = foundNoArg;
#ifdef GSDebugFile
			arg = NULL;
			fprintf(GSDebugFile, "### foundNoArg: argCount %d, argList NULL\n", *pArgCount);
#endif
			}
		else {
#ifdef GSDebugFile
			fprintf(GSDebugFile, "### Parsing arg '%s' (argCount %d)...\n", arg, *pArgCount);
			fflush(GSDebugFile);
#endif
			pState = NULL;
			switch(arg[0]) {
				case '\0':
					stateFound = foundNullArg;
					break;
				case '-':
					switch(arg[1]) {
						case '-':
							if(arg[2] == '\0') {
								stateFound = foundNoArg;
								--*pArgCount;
								++*pArgList;
								}
							else
								goto BumpArg;
							break;
						case '\0':
							stateFound = foundArg;
							break;
						default:
							stateFound = foundSwitch;
						}
					break;
				case '+':
					if(arg[1] == '+' && arg[2] != '\0') {
BumpArg:
						stateFound = foundArg;
						++arg;
						break;
						}
					if(isdigit(arg[1])) {
						hashTable = state.hashTable;
						while((pHashRec = heach(&hashTable)) != NULL) {
							pSwitch = (pState = ssPtr(pHashRec))->pSwitch;
							if((pSwitch->flags & (SF_NumericSwitch | SF_PlusType)) ==
							 (SF_NumericSwitch | SF_PlusType)) {
								if(*strpspn(arg + 2, digits) == '\0') {
									stateFound = foundSwitch;
									goto FoundDone;
									}
								goto BadNumericSwitch;
								}
							}
						}
					// else fall through.
				default:
					stateFound = foundArg;
				}
			}
FoundDone:
#ifdef GSDebugFile
		fprintf(GSDebugFile, "*** Found arg '%s', stateFound %d, argCount %d, argListNext '%s'.\n", arg, stateFound,
		 *pArgCount, *pArgCount == 0 ? "NULL" : *(*pArgList + 1));
		fflush(GSDebugFile);
#endif
		// Compare expected and found states.  If found state is foundSwitch and pState is NULL, need to search for
		// switch in hash table.
		switch(stateFound) {
			case foundNoArg:

				// No arguments left.
				switch(stateExpected) {
					case expectOptionalArg:
						goto NoArgReturn;
					case expectRequiredArg:
						goto ValueRequired;
					default:	// expectSwitch
						pResult->name = pResult->value = NULL;
						goto NoSwitchesLeft;
					}
			case foundSwitch:

				// Switch found.
				switch(stateExpected) {
					case expectOptionalArg:
						goto NoArgReturn;
					case expectRequiredArg:
						goto ValueRequired;
					default:	// expectSwitch
						argSwitch = arg + 1;
						if(*arg == '+') {
							tableSwitch = NSPlusKey;
							goto MatchFound;
							}
					}
#ifdef GSDebugFile
				fprintf(GSDebugFile, "*** Checking hash table (size %lu, entries %lu) for '%s'...\n",
				 state.hashTable->hashSize, state.hashTable->recCount, argSwitch);
				fflush(GSDebugFile);
#endif
				// Check hash table for a match.
				if(pState != NULL)
					goto SetName;
				if((pHashRec = hsearch(state.hashTable, argSwitch)) != NULL) {
					pState = ssPtr(pHashRec);
SetName:
					tableSwitch = *pState->pSwitch->names;
					goto MatchFound;
					}
#ifdef GSDebugFile
				fprintf(GSDebugFile, "### No switch found matching argSwitch '%s'.\n", argSwitch);
				fflush(GSDebugFile);
#endif
				// No match found.  Check if valid numeric switch (-n type).
				hashTable = state.hashTable;
				while((pHashRec = heach(&hashTable)) != NULL) {
					pSwitch = (pState = ssPtr(pHashRec))->pSwitch;
					if((pSwitch->flags & (SF_NumericSwitch | SF_PlusType)) == SF_NumericSwitch) {
						if(*strpspn(argSwitch, digits) == '\0') {
							tableSwitch = NSMinusKey;
							goto MatchFound;
							}
						if(isdigit(*argSwitch))
							goto BadNumericSwitch;
						break;
						}
					}

				// No numeric switch in switch table or wrong type.
				goto UnknownSwitch;
MatchFound:
#ifdef GSDebugFile
				fprintf(GSDebugFile, "### Match found, checking if -%s is a duplicate...\n", tableSwitch);
				fflush(GSDebugFile);
#endif
				// Found match; check for duplicate (pState and tableSwitch are set).
				pSwitch = pState->pSwitch;
				if(++pState->foundCount > 1 && !(pSwitch->flags & SF_AllowRepeat))
					goto DupNotAllowed;
#ifdef GSDebugFile
				fputs("### Not a duplicate, updating argument pointers...\n", GSDebugFile);
				fflush(GSDebugFile);
#endif
				// Not a duplicate or repeat allowed; update argument pointers.
				--*pArgCount;
				++*pArgList;
				switchIndex = pSwitch - state.pSwitch + 1;

#ifdef GSDebugFile
				fprintf(GSDebugFile, "### Checking switch type (switchIndex %d)...\n", switchIndex);
				fflush(GSDebugFile);
#endif
				// Numeric switch?
				if(pSwitch->flags & SF_NumericSwitch)
					goto NumericSwitchReturn;

				// Simple switch?
				if((argType = pSwitch->flags & SF_ArgMask) == SF_NoArg)
					goto NoArgReturn;

				// Keyword switch.
				stateExpected = (argType == SF_OptionalArg) ? expectOptionalArg : expectRequiredArg;
				break;
			case foundNullArg:

				// Null argument found.
				if(stateExpected == expectSwitch)
					goto NoSwitchesLeft;

				// Validate argument.
				if(!(pSwitch->flags & SF_AllowNullArg))
					goto NullNotAllowed;

				// Return keyword switch and argument.
				--*pArgCount;
				++*pArgList;
				goto ArgReturn;
			case foundArg:

				// Non-null argument found.
				if(stateExpected == expectSwitch)
					goto NoSwitchesLeft;

				// Validate argument.
				if(pSwitch->flags & SF_NumericArg) {
					str0 = arg;
					if(*str0 == '-' || *str0 == '+') {
						if(!(pSwitch->flags & SF_AllowSign))
							goto MustBeUnsigned;
						++str0;
						}
					if((str1 = strpspn(str0, digits)) == str0)
						goto MustBeNumeric;
					if(*str1 != '\0') {
						if(*str1 != '.')
							goto MustBeNumeric;
						if(!(pSwitch->flags & SF_AllowDecimal))
							goto MustBeInteger;
						if(*strpspn(str1 + 1, digits) != '\0')
							goto MustBeNumeric;
						}
					}

				// Return keyword switch and argument.
				--*pArgCount;
				++*pArgList;
				goto ArgReturn;
			}
		}

NoSwitchesLeft:

	// No switch arguments left; check for required switch(es).
	hashTable = state.hashTable;
	while((pHashRec = heach(&hashTable)) != NULL) {
		pSwitch = (pState = ssPtr(pHashRec))->pSwitch;
		if((pSwitch->flags & SF_RequiredSwitch) && pState->foundCount == 0)
			return (pSwitch->flags & SF_NumericSwitch) ? emsgf(-1, "Numeric (%c) switch required",
			 (pSwitch->flags & SF_PlusType) ? '+' : '-') : emsgf(-1, "-%s switch required", pSwitch->names[0]);
		}

	// Scan completed.  Delete SwitchState objects from hash, delete hash, reset static pointer, and return result.
	for(pSwitchEnd = (pSwitch = state.pSwitch) + switchCount; pSwitch < pSwitchEnd; ++pSwitch) {
		pHashRec = hsearch(state.hashTable, !(pSwitch->flags & SF_NumericSwitch) ? pSwitch->names[0] :
		 (pSwitch->flags & SF_PlusType) ? NSPlusKey : NSMinusKey);
		free((void *) ssPtr(pHashRec));
		}
	hfree(state.hashTable);
	state.pSwitch = NULL;
	return 0;

	// Switch found.
NumericSwitchReturn:
#ifdef GSDebugFile
	fprintf(GSDebugFile, "### NumSwRtn: arg '%s'\n", arg);
	fflush(GSDebugFile);
#endif
	pResult->value = arg;
	argSwitch = NULL;
	goto SwitchFound;
NoArgReturn:
	pResult->value = NULL;
	goto SwitchFound;
ArgReturn:
	pResult->value = arg;
SwitchFound:
	pResult->name = argSwitch;
#ifdef GSDebugFile
	fprintf(GSDebugFile, "### RETURNING name '%s', value '%s', index %d...\n", pResult->name, pResult->value, switchIndex);
	fflush(GSDebugFile);
#endif
	return switchIndex;

	// Exception return.
MustBeNumeric:
	str0 = "numeric";
	goto MustBe;
MustBeUnsigned:
	str0 = "unsigned";
	goto MustBe;
MustBeInteger:
	str0 = "an integer";
MustBe:
	return emsgf(-1, "-%s switch value '%s' must be %s", argSwitch, arg, str0);
UnknownSwitch:
	return emsgf(-1, "Unknown switch, -%s", argSwitch);
BadNumericSwitch:
	return emsgf(-1, "Invalid numeric switch, %s", arg);
DupNotAllowed:
	return (pSwitch->flags & SF_NumericSwitch) ? emsgf(-1, "Duplicate numeric switch, %s", arg) :
	 emsgf(-1, "Duplicate switch, -%s", argSwitch);
ValueRequired:
	return emsgf(-1, "-%s switch requires a value", argSwitch);
NullNotAllowed:
	return emsgf(-1, "-%s switch value cannot be null", argSwitch);
	}
