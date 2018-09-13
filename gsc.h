#ifndef GSC_H
#define GSC_H

#include "net/nanocoap.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GSC_MAX_URLS
#define GSC_MAX_URLS (10)
#endif

typedef struct gsc_t {
    int devno;
    const char *type;
    char td_url[NANOCOAP_URL_MAX];
    char val_url[NANOCOAP_URL_MAX];
} gsc_t;

#ifdef __cplusplus
}
#endif

#endif /* GSC_H */
/** @} */
