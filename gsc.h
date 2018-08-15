#ifndef GSC_H
#define GSC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Struct pairing to hold information about
 * registered resource
 */
typedef struct gsc_rsc {
    char url[NANOCOAP_URL_MAX];		/**< URL of device */
    uint8_t num;			/**< Corresponding device */
} gsc_t;

#ifdef __cplusplus
}
#endif

#endif
