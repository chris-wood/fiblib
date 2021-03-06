#include "fib_tbf.h"

#include "map.h"
#include "patricia.h"
#include "bloom.h"

typedef enum {
    _FIBEntryType_Bitmap,
    _FIBEntryType_BF,
} _FIBEntryType;

typedef struct {
    int type;
    Bitmap *vector;

    BloomFilter **filters;
    int numFilters;
} _fibEntry;

static void
_fibEntry_Destroy(_fibEntry **entryP)
{
    _fibEntry *entry = *entryP;

    if (entry->vector != NULL) {
        bitmap_Destroy(&entry->vector);
    }
    for (int i = 0; i < entry->numFilters; i++) {
        bloom_Destroy(&(entry->filters[i]));
    }
    if (entry->filters != NULL) {
        free(entry->filters);
    }

    free(entry);
    *entryP = NULL;
}

static void
_fibEntry_AssertIsValid(_fibEntry *entry)
{
    assertTrue(entry->type == _FIBEntryType_Bitmap || entry->type == _FIBEntryType_BF, "Invalid entry type");

    if (entry->type == _FIBEntryType_Bitmap) {
        assertTrue(entry->vector != NULL, "Invalid entry vector");
    } else {
        assertTrue(entry->filters != NULL, "Invalid entry filters");
    }

}

static _fibEntry *
_fibEntry_Create(int type)
{
    _fibEntry *entry = (_fibEntry *) malloc(sizeof(_fibEntry));
    if (entry != NULL) {
        entry->type = type;
        entry->vector = NULL;
        entry->filters = NULL;
        entry->numFilters = 0;
    }
    return entry;
}

struct fib_tbf {
    int T;
    int m;
    int k;
    Patricia *trie;
    Map *map;
};

#define MIN(a, b) (a < b ? a : b)

Bitmap *
fibTBF_LPM(FIBTBF *fib, const Name *name)
{
    int numSegments = name_GetSegmentCount(name);
    bool isShortName = numSegments <= fib->T;

    PARCBuffer *tSegment = name_GetWireFormat(name, MIN(fib->T, numSegments));
    _fibEntry *entry = patricia_Get(fib->trie, tSegment);

    parcBuffer_Release(&tSegment);
    if (entry == NULL) {
        return NULL;
    }

    _fibEntry_AssertIsValid(entry);
    if (isShortName) {
        return entry->vector;
    } else {
        // Lookup prefix with the BFs
        BloomFilter **maps = entry->filters;

        // Find the longest prefix
        int i = 0;
        for (i = MIN(entry->numFilters + fib->T, numSegments); i > fib->T; i--) {
            PARCBuffer *subPrefix = name_GetSubWireFormat(name, fib->T, i);
            if (bloom_Test(entry->filters[i - fib->T - 1], subPrefix)) {
                parcBuffer_Release(&subPrefix);
                break;
            }
            parcBuffer_Release(&subPrefix);
        }

        // We didn't find a hit in the bloom filters, so return the vector associated with the trie lookup
        if (i == fib->T) {
            return entry->vector;
        } else {
            PARCBuffer *matchingPrefix = name_GetWireFormat(name, i);
            void *result = map_Get(fib->map, matchingPrefix);
            parcBuffer_Release(&matchingPrefix);
            return result;
        }
    }

    return NULL;
}

bool
fibTBF_Insert(FIBTBF *fib, const Name *name, Bitmap *egressVector)
{
    name_AssertIsValid(name);

    int numSegments = name_GetSegmentCount(name);
    bool isShortName = numSegments <= fib->T;

    PARCBuffer *tSegment = name_GetWireFormat(name, MIN(fib->T, numSegments));
    _fibEntry *entry = patricia_Get(fib->trie, tSegment);

    if (entry != NULL) {
        if (isShortName) { // if it's a short name, then insert into the BitMap
            bitmap_SetVector(entry->vector, egressVector);
        } else { // else, insert into the BF
            int B = numSegments - fib->T;
            assertTrue(B > 0, "A BF entry must have at least T segments");

            if (B > entry->numFilters) {
                entry->filters = (BloomFilter **) realloc(entry->filters, (B + 1) * sizeof(BloomFilter *));
                for (int i = entry->numFilters; i <= B; i++) {
                    entry->filters[i] = bloom_Create(fib->m, fib->k);
                }
                entry->numFilters = B;
            }

            PARCBuffer *suffix = name_GetSubWireFormat(name, fib->T, numSegments);
            bloom_Add(entry->filters[B], suffix);
            parcBuffer_Release(&suffix);

            PARCBuffer *entireName = name_GetWireFormat(name, numSegments);
            map_Insert(fib->map, entireName, egressVector);
            parcBuffer_Release(&entireName);
        }
    } else {
        _fibEntry *newEntry = NULL;
        if (isShortName) {
            newEntry = _fibEntry_Create(_FIBEntryType_Bitmap);
            newEntry->vector = egressVector;
        } else {
            newEntry = _fibEntry_Create(_FIBEntryType_BF);
            newEntry->vector = egressVector;
            int B = numSegments - fib->T;

            newEntry->filters = (BloomFilter **) malloc((B + 1) * sizeof(BloomFilter *));
            for (int i = 0; i <= B; i++) {
                newEntry->filters[i] = bloom_Create(fib->m, fib->k);
            }
            newEntry->numFilters = B;

            PARCBuffer *suffix = name_GetSubWireFormat(name, fib->T, numSegments);
            bloom_Add(newEntry->filters[B], suffix);
            parcBuffer_Release(&suffix);

            PARCBuffer *entireName = name_GetWireFormat(name, numSegments);
            map_Insert(fib->map, entireName, egressVector);
            parcBuffer_Release(&entireName);
        }

        patricia_Insert(fib->trie, tSegment, newEntry);
    }

    parcBuffer_Release(&tSegment);

    // If N < T, insert item that contains bitmap
    // Else, insert item that contains PBF
    // for both, append the items created to a list so they can be deleted later on
    return true;
}

void
fibTBF_Destroy(FIBTBF **fibP)
{
    FIBTBF *fib = *fibP;

    patricia_Destroy(&fib->trie);
    map_Destroy(&fib->map);

    free(fib);
    *fibP = NULL;
}

FIBTBF *
fibTBF_Create(int T, int m, int k)
{
    FIBTBF *fib= (FIBTBF *) malloc(sizeof(FIBTBF));
    if (fib != NULL) {
        fib->T = T;
        fib->m = m;
        fib->k = k;
        fib->trie = patricia_Create(_fibEntry_Destroy);
        fib->map = map_Create(bitmap_Destroy);
    }
    return fib;
}

FIBInterface *TBFAsFIB = &(FIBInterface) {
        .LPM = (Bitmap *(*)(void *instance, const Name *ccnxName)) fibTBF_LPM,
        .Insert = (bool (*)(void *instance, const Name *ccnxName, Bitmap *vector)) fibTBF_Insert,
        .Destroy = (void (*)(void **instance)) fibTBF_Destroy,
};

