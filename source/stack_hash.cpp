#include "include/stack_hash.h"

uint64_t qhashfnv1_64(const void *data, size_t nbytes) {
    if (data == NULL || nbytes == 0)
        return 0;

    unsigned char *dp;
    uint64_t h = 0xCBF29CE484222325ULL;

    for (dp = (unsigned char *) data; nbytes > 0; dp++, nbytes--) {
#ifdef __GNUC__
        h += (h << 1) + (h << 4) + (h << 5) +
        (h << 7) + (h << 8) + (h << 40);
#else
        h *= 0x100000001B3ULL;
#endif
        h ^= *dp;
    }

    return h;
}
