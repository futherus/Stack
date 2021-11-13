/** \file 
 *  \brief Header with external hash function
 */
#ifndef STACK_HASH_H
#define STACK_HASH_H

#include <stdint.h>

/**
 * Get 64-bit FNV1 hash integer.
 *
 * @param data      source data
 * @param nbytes    size of data
 *
 * @return 64-bit unsigned hash value.
 *
 * @code
 *   uint64_t fnv64 = qhashfnv1_64((void*)"hello", 5);
 * @endcode
 */
uint64_t qhashfnv1_64(const void *data, size_t nbytes);

#endif // STACK_HASH_H
