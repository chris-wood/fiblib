#ifdef __cplusplus
extern "C" {
#endif

#ifndef patricia_h_
#define patricia_h_

#include <parc/algol/parc_Buffer.h>

struct patricia;
typedef struct patricia Patricia;

Patricia *patricia_Create(void (*valueDestructor)(void **valueP));

void patricia_Destroy(Patricia **patricia);

void patricia_Insert(Patricia *trie, PARCBuffer *key, void *item);

void *patricia_Get(Patricia *trie, PARCBuffer *key);

void patricia_Display(Patricia *trie);

#endif // patricia_h_

#ifdef __cplusplus
}
#endif
