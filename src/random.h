#ifdef __cplusplus
extern "C" {
#endif

#ifndef random_h_
#define random_h_

#include <stdint.h>

#include <parc/algol/parc_Buffer.h>

PARCBuffer *random_Bytes(PARCBuffer *buffer);

#endif // random_h_

#ifdef __cplusplus
}
#endif
