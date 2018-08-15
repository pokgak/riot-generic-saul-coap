#ifndef DSC_H
#define DSC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Struct pairing to hold corresponding pair of URI
 * with the device num together
 */
typedef struct {
    char url[NANOCOAP_URL_MAX];		/**< URL of device */
    uint8_t num;			/**< Corresponding device */
} saul_coap_t;

#ifdef __cplusplus
}
#endif

#endif
