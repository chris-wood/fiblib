//
// Created by Christopher Wood on 12/5/16.
//

#include "sha256hasher.h"

#include <parc/security/parc_CryptoHasher.h>

struct sha256hasher {
    PARCCryptoHasher *hasher;
};

SHA256Hasher *
sha256hasher_Create()
{
    SHA256Hasher *hasher = (SHA256Hasher *) malloc(sizeof(SHA256Hasher));
    if (hasher != NULL) {
        hasher->hasher = parcCryptoHasher_Create(PARCCryptoHashType_SHA256);
    }
    return hasher;
}

void
sha256hasher_Destroy(SHA256Hasher **hasherP)
{
    SHA256Hasher *hasher = *hasherP;
    parcCryptoHasher_Release(&hasher->hasher);
    free(hasher);
    *hasherP = NULL;
}

PARCBuffer *
sha256hasher_Hash(SHA256Hasher *hasher, PARCBuffer *input)
{
    parcCryptoHasher_Init(hasher->hasher);
    parcCryptoHasher_UpdateBuffer(hasher->hasher, input);
    PARCCryptoHash *hash = parcCryptoHasher_Finalize(hasher->hasher);
    PARCBuffer *digest = parcBuffer_Acquire(parcCryptoHash_GetDigest(hash));
    parcCryptoHash_Release(&hash);
    return digest;
}

PARCBuffer *
sha256hasher_HashArray(SHA256Hasher *hasher, size_t length, uint8_t input[length])
{
    parcCryptoHasher_Init(hasher->hasher);
    parcCryptoHasher_UpdateBytes(hasher->hasher, input, length);
    PARCCryptoHash *hash = parcCryptoHasher_Finalize(hasher->hasher);
    PARCBuffer *digest = parcBuffer_Acquire(parcCryptoHash_GetDigest(hash));
    parcCryptoHash_Release(&hash);
    return digest;
}

Bitmap *
sha256hasher_HashToVector(SHA256Hasher *hasher, PARCBuffer *input, int range)
{
    Bitmap *vector = bitmap_Create(range);

    PARCBuffer *hashOutput = sha256hasher_Hash(hasher, input);
    size_t index = parcBuffer_GetUint64(hashOutput) % range;
    bitmap_Set(vector, index);
    parcBuffer_Release(&hashOutput);

    return vector;
}

Bitmap *
sha256hasher_HashArrayToVector(SHA256Hasher *hasher, size_t length, uint8_t input[length], int range)
{
    Bitmap *vector = bitmap_Create(range);

    PARCBuffer *hashOutput = sha256hasher_HashArray(hasher, length, input);
    size_t index = parcBuffer_GetUint64(hashOutput) % range;
    bitmap_Set(vector, index);
    parcBuffer_Release(&hashOutput);

    return vector;
}

HasherInterface *SHA256HashAsHasher = &(HasherInterface) {
        .Hash = (PARCBuffer *(*)(void *, PARCBuffer *)) sha256hasher_Hash,
        .HashArray = (PARCBuffer *(*)(void *hasher, size_t length, uint8_t *input)) sha256hasher_HashArray,
        .HashToVector = (Bitmap *(*)(void*hasher, PARCBuffer *input, int range)) sha256hasher_HashToVector,
        .HashArrayToVector = (Bitmap *(*)(void*hasher, size_t length, uint8_t *input, int range)) sha256hasher_HashArrayToVector,
        .Destroy = (void (*)(void **instance)) sha256hasher_Destroy,
};
