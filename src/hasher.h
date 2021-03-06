#ifdef __cplusplus
extern "C" {
#endif

#ifndef FIB_PERF_HASHER_H
#define FIB_PERF_HASHER_H

#include "bitmap.h"

#include <parc/algol/parc_Buffer.h>

struct hasher;
typedef struct hasher Hasher;

typedef struct {
    PARCBuffer *(*Hash)(void *hasher, PARCBuffer *input);

    PARCBuffer *(*HashArray)(void *hasher, size_t length, uint8_t input[length]);

    Bitmap *(*HashToVector)(void *hasher, PARCBuffer *input, int range);

    Bitmap *(*HashArrayToVector)(void *hasher, size_t length, uint8_t input[length], int range);

    void (*Destroy)(void **instance);
} HasherInterface;

Hasher *hasher_Create(void *instance, HasherInterface *interface);

void hasher_Destroy(Hasher **hasherP);

PARCBuffer *hasher_Hash(Hasher *hasher, PARCBuffer *input);

PARCBuffer *hasher_HashTruncated(Hasher *hasher, PARCBuffer *input, int limit);

PARCBuffer *hasher_HashArray(Hasher *hasher, size_t length, uint8_t input[length]);

Bitmap *hasher_HashToVector(Hasher *hasher, PARCBuffer *input, int range);

Bitmap *hasher_HashArrayToVector(Hasher *hasher, size_t length, uint8_t input[length], int range);

#endif //FIB_PERF_HASHER_H

#ifdef __cplusplus
}
#endif

