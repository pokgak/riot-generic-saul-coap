#ifndef GSC_H
#define GSC_H

#include "net/nanocoap.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GSC_MAX_URLS
#define GSC_MAX_URLS (10)	/*<Max. devices allowed to be registered */
#endif

/* Information of registered device */
typedef struct gsc_t {
    int devno;				/* Device ID as registered by SAUL */
    const char *type;			/* Type of the device (Actuator/Sensor) */
    char td_url[NANOCOAP_URL_MAX];	/* URL for Thing Description */
    char val_url[NANOCOAP_URL_MAX];	/* URL of reading of the state/sensor */
} gsc_t;

/*
 * Parses URL for device num. Use this function together with
 * saul_reg_find_nth to retrieve device from the global saul registry.
 */
int get_devnum(const char *url);

/* Returns the type of the device (Actuator/Sensor) */
const char *get_type(const char *url);

/*
 * Adds dev to the resource array _resources.
 * Parses the URI from the SAUL_CLASS.
 * Uses _generic_handler for the handler.
 */
int gsc_init(coap_resource_t *resources);

#ifdef __cplusplus
}
#endif

#endif /* GSC_H */
/** @} */
