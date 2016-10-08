#include <stdio.h>

#include "fib_cisco.h"
#include "map.h"

#include <parc/algol/parc_SafeMemory.h>

typedef struct {
    bool isVirtual;
    int maxDepth;
    PARCBuffer *buffer;
    PARCBitVector *vector;
} _FIBCiscoEntry;

struct fib_cisco {
    int M;
    int numMaps;
    Map **maps;
};

static _FIBCiscoEntry *
_fibCisco_CreateVirtualEntry(int depth)
{
    _FIBCiscoEntry *entry = (_FIBCiscoEntry *) malloc(sizeof(_FIBCiscoEntry));
    if (entry != NULL) {
        entry->isVirtual = true;
        entry->maxDepth = depth;
        entry->vector = NULL;
        entry->buffer = NULL;
    }
    return entry;
}

static _FIBCiscoEntry *
_fibCisco_CreateEntry(PARCBitVector *vector, PARCBuffer *buffer, int depth)
{
    _FIBCiscoEntry *entry = (_FIBCiscoEntry *) malloc(sizeof(_FIBCiscoEntry));
    if (entry != NULL) {
        entry->isVirtual = false;
        entry->maxDepth = depth;
        entry->vector = parcBitVector_Acquire(vector);;
        entry->buffer = buffer;
    }
    return entry;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static PARCBuffer *
_computeNameBuffer(FIBCisco *fib, const CCNxName *prefix, int numSegments)
{
    CCNxName *copy = ccnxName_Copy(prefix);
    int length = ccnxName_GetSegmentCount(copy);
    if (length > numSegments) {
        ccnxName_Trim(copy, length - numSegments);
    }

    char *nameString = ccnxName_ToString(copy);
    PARCBuffer *buffer = parcBuffer_AllocateCString(nameString);
    parcMemory_Deallocate(&nameString);
    ccnxName_Release(&copy);
   
    return buffer;
}

static _FIBCiscoEntry *
_lookupNamePrefix(FIBCisco *fib, const CCNxName *prefix, int numSegments)
{
    PARCBuffer *buffer = _computeNameBuffer(fib, prefix, numSegments);
    _FIBCiscoEntry *entry = map_Get(fib->maps[numSegments - 1], buffer);
    parcBuffer_Release(&buffer);
    return entry;
}

static void
_insertNamePrefix(FIBCisco *fib, const CCNxName *prefix, int numSegments, _FIBCiscoEntry *entry)
{
    PARCBuffer *buffer = _computeNameBuffer(fib, prefix, numSegments);
    map_Insert(fib->maps[numSegments - 1], buffer, entry);
    parcBuffer_Release(&buffer);
}

PARCBitVector *
fibCisco_LPM(FIBCisco *fib, const CCNxName *name)
{
    int numSegments = ccnxName_GetSegmentCount(name);

    // prefixCount = min(numSegments, M)
    int prefixCount = MIN(MIN(fib->M, numSegments), fib->numMaps);
    int startPrefix = MIN(numSegments, fib->numMaps);

    _FIBCiscoEntry *firstEntryMatch = NULL;
    for (int i = prefixCount; i > 0; i--) {
        _FIBCiscoEntry *entry = _lookupNamePrefix(fib, name, i); 

        if (entry != NULL) {
            if (entry->maxDepth > fib->M && numSegments > fib->M) {
                startPrefix = MIN(numSegments, entry->maxDepth); 
                firstEntryMatch = entry;
                break;
            } else if (!entry->isVirtual) {
                return entry->vector;
            } 
        }
    }

    if (firstEntryMatch == NULL) {
        return NULL;
    }

    _FIBCiscoEntry *entry = NULL;
    for (int i = startPrefix; i >= 1; i--) {
        if (startPrefix == fib->M) {
            entry = firstEntryMatch;
            if (!entry->isVirtual) {
                return entry->vector;
            }
        }

        _FIBCiscoEntry *targetEntry = _lookupNamePrefix(fib, name, i); 
        if (targetEntry != NULL && !targetEntry->isVirtual) {
            return targetEntry->vector;
        } 
    }

    return NULL;
}

static Map *
_fibCisco_CreateMap()
{
    return map_CreateWithLinkedBuckets(MapOverflowStrategy_OverflowBucket, true);
}

static void
_fibCisco_ExpandMapsToSize(FIBCisco *fib, int number)
{
    if (fib->numMaps < number) {
        fib->maps = (Map **) realloc(fib->maps, number * (sizeof(Map *)));
        for (size_t i = fib->numMaps; i <= number; i++) {
            fib->maps[i] = _fibCisco_CreateMap();
        }
        fib->numMaps = number;
    }
}

bool
fibCisco_Insert(FIBCisco *fib, const CCNxName *name, PARCBitVector *vector)
{
    size_t numSegments = ccnxName_GetSegmentCount(name);

    /*
    PARCBitVector *entry = fibCisco_LPM(fib, name);
    if (entry != NULL) {
        entry->isVirtual = false;
        entry->maxDepth = numSegments;
        return true;
    }
    */

    _fibCisco_ExpandMapsToSize(fib, numSegments);

    // Check to see if we need to create a virtual FIB entry.
    // This occurs when numSegments > M
    // If a FIB entry already exists for M segments, real or virtual, update the MD
    size_t maximumDepth = numSegments;
    if (numSegments > fib->M) {
        _FIBCiscoEntry *entry = _lookupNamePrefix(fib, name, fib->M); 

        if (entry == NULL) {
            entry = _fibCisco_CreateVirtualEntry(maximumDepth);
            _insertNamePrefix(fib, name, fib->M, entry);
        } else if (entry->maxDepth < maximumDepth) {
            entry->maxDepth = MAX(entry->maxDepth, maximumDepth);
        }
    }

    // Update the MD for all segments smaller
    /*
    for (size_t i = 1; i < numSegments; i++) {
        _FIBCiscoEntry *entry = _lookupNamePrefix(fib, name, i);
        if (entry != NULL) {
            entry->maxDepth = MAX(entry->maxDepth, maximumDepth);
        }
    }
    */

    PARCBuffer *buffer = _computeNameBuffer(fib, name, numSegments);
    _FIBCiscoEntry *entry = _fibCisco_CreateEntry(vector, buffer, maximumDepth);
    parcBuffer_Release(&buffer);
    
    // If there is an existing entry, make sure it's *NOT* virtual and update its MD if necessary
    _FIBCiscoEntry *existingEntry = _lookupNamePrefix(fib, name, numSegments);
    if (existingEntry != NULL) {
        existingEntry->isVirtual = false;
        existingEntry->maxDepth = MAX(numSegments, existingEntry->maxDepth);
        if (existingEntry->vector == NULL) {
            existingEntry->vector = parcBitVector_Acquire(vector);
        } else {
            parcBitVector_SetVector(existingEntry->vector, vector);
        }
    } else {
        _insertNamePrefix(fib, name, numSegments, entry);
    }

    return false;
}

FIBCisco *
fibCisco_Create(int M)
{
    FIBCisco *native = (FIBCisco *) malloc(sizeof(FIBCisco));
    if (native != NULL) {
        native->M = M;
        native->maps = (Map **) malloc(sizeof(Map *));
        native->numMaps = 1;
        native->maps[0] = _fibCisco_CreateMap();
    }

    return native;
}

FIBInterface *CiscoFIBAsFIB = &(FIBInterface) {
    .LPM = (PARCBitVector *(*)(void *instance, const CCNxName *ccnxName)) fibCisco_LPM,
    .Insert = (bool (*)(void *instance, const CCNxName *ccnxName, PARCBitVector *vector)) fibCisco_Insert,
};
