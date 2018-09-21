#include <string.h>
#include "assert.h"
#include "memarray.h"
#include "cn-cbor/cn-cbor.h"

void *cbor_calloc(size_t count, size_t size, void *memblock)
{
    (void)count;
    assert(count == 1);
    void *block = memarray_alloc(memblock);
    if (block) {
        memset(block, 0, size);
    }
    return block;
}

void cbor_free(void *ptr, void *memblock)
{
    memarray_free(memblock, ptr);
}
