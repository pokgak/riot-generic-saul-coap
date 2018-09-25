#include <stdio.h>
#include <string.h>
#include "assert.h"
#include "memarray.h"
#include "cn-cbor/cn-cbor.h"

static void _handle_cbor_type(cn_cbor *cbor);
static void _parse_cbor_map(cn_cbor *map);

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

void parse_cbor(cn_cbor *root)
{
    if (root->type == CN_CBOR_MAP) {
        printf("\n");
        _parse_cbor_map(root);
        printf("\n");
    }
    else {
        puts("CBOR root started not with map");
    }

    fflush(stdout);
    fflush(stdout);
}

static void _handle_cbor_type(cn_cbor *cbor)
{
    switch (cbor->type) {
    case CN_CBOR_FALSE:
        printf("\"false\""); break;
    case CN_CBOR_TRUE:
        printf("\"true\""); break;
    case CN_CBOR_UINT:
        printf("%lu", cbor->v.uint); break;
    case CN_CBOR_INT:
        printf("%ld", cbor->v.sint); break;
    case CN_CBOR_TEXT:
        printf("\"%.*s\"", cbor->length, cbor->v.str); break;
    case CN_CBOR_MAP:
        _parse_cbor_map(cbor); break;
    default:
        puts("UNHANDLED TYPE");
    }
}

static void _parse_cbor_map(cn_cbor *map)
{
    cn_cbor *child = map->first_child;
    int comma = 0;
    printf("{");

    while (child) {
        _handle_cbor_type(child);
        child = child->next;

        if (child) {
            if ((comma % 2) != 0) {
                printf(", ");
            }
            else {
                printf(": ");
            }
        }
        comma++;
    }

    printf("}");
}
