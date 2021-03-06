#ifdef __cplusplus
extern "C" {
#endif

#ifndef FIB_PERF_PREFIX_BLOOM_H
#define FIB_PERF_PREFIX_BLOOM_H

#include "name.h"

// A Prefix Bloom Filter, as described by Varvello et al. in
//   http://conferences.sigcomm.org/sigcomm/2012/paper/icn/p73.pdf
// is a data structure that takes a list of strings as input
// and returns the index j in the list such that i=1,..,j elements
// are "in" the filter.

struct prefix_bloom_filter;
typedef struct prefix_bloom_filter PrefixBloomFilter;

PrefixBloomFilter *prefixBloomFilter_Create(int b, int m, int k);

void prefixBloomFilter_Destroy(PrefixBloomFilter **bfP);

void prefixBloomFilter_Add(PrefixBloomFilter *filter, const Name *name);

int prefixBloomFilter_LPM(PrefixBloomFilter *filter, const Name *name);

#endif //FIB_PERF_PREFIX_BLOOM_H

#ifdef __cplusplus
}
#endif
