#include <stdio.h>
#include <ctype.h>
#include "net/gcoap.h"
#include "fmt.h"
#include "saul_reg.h"

#define NUM_URLS (15)

extern ssize_t get_td(char *out, const char *url);

static char _td_urls[NUM_URLS][NANOCOAP_URL_MAX];
static char _val_urls[NUM_URLS][NANOCOAP_URL_MAX];

/*
 * Parses URL for device num. Used to retrieve device from saul registry
 */
int _get_devnum(const char *url)
{
    char num[4];
    /* skip the first '/' in url */
    const char *start = url + 1;
    const char *last = strchr(start, '/');
    snprintf(num, (ssize_t) (last - start + 1), "%s", start);
    return atoi((const char *) num);
}

/*
 * Parse the (multidimensional) result res from given phydat_t and
 * write the value to the char *data
 */
static ssize_t _parse_res(uint8_t dim, phydat_t *res, char *data, size_t datalen)
{
    if (res == NULL || dim > PHYDAT_DIM) {
        puts("Unable to display data object");
        return -1;
    }
    sprintf(data, "Data:");
    for (uint8_t i = 0; i < dim; i++) {
        char scale_prefix;

        switch (res->unit) {
            case UNIT_UNDEF:
            case UNIT_NONE:
            case UNIT_M2:
            case UNIT_M3:
            case UNIT_PERCENT:
            case UNIT_TEMP_C:
            case UNIT_TEMP_F:
                /* no string conversion */
                scale_prefix = '\0';
                break;
            default:
                scale_prefix = phydat_prefix_from_scale(res->scale);
        }

        sprintf(data + strlen(data), "\t");
        if (dim > 1) {
            sprintf(data + strlen(data), "[%u] ", (unsigned int)i);
        }
        else {
            sprintf(data + strlen(data), "     ");
        }
        if (scale_prefix) {
            sprintf(data + strlen(data), "%6d %c", (int)res->val[i], scale_prefix);
        }
        else if (res->scale == 0) {
            sprintf(data + strlen(data), "%6d", (int)res->val[i]);
        }
        else if ((res->scale > -5) && (res->scale < 0)) {
            char num[8];
            size_t len = fmt_s16_dfp(num, res->val[i], res->scale);
            num[len] = '\0';
            sprintf(data + strlen(data), "%s", num);
        }
        else {
            sprintf(data + strlen(data), "%iE%i", (int)res->val[i], (int)res->scale);
        }

        sprintf(data + strlen(data), "%s\n", phydat_unit_to_str(res->unit));
    }

    if (strlen(data) > datalen)
        return -1;

    return 0;
}

static ssize_t _generic_td_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;

    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));
    if (method_flag == COAP_PUT || method_flag == COAP_POST) {
        return -1;
    }

    gcoap_resp_init(pdu, buf,  len, COAP_CODE_CONTENT);
    ssize_t td_len = get_td((char *)pdu->payload, (const char *)pdu->url);

    return gcoap_finish(pdu, td_len, COAP_FORMAT_TEXT);
}

/*
 * Generic handler for the resources. Accepts either a GET or a PUT.
 * Only read or write values for now.
 */
static ssize_t _generic_val_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;
    int dim;

    /* find which sensor we are currently dealing with */
    saul_reg_t *dev;
    int num = _get_devnum((const char *)pdu->url);
    if (num < 0) {
	    printf("_generic_handler: cannot find dev with URI %s\n", pdu->url);
	    return -1;
    }

    dev = saul_reg_find_nth(num);
    if (dev == NULL) {
        puts("error: undefined device id (num) given");
        return -1;
    }

    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));

    size_t datalen = 50;
    char data[datalen];
    switch(method_flag) {
        case COAP_GET:
            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);

	    /* read from the device */
	    phydat_t res;

	    dim = saul_reg_read(dev, &res);
	    if (dim <= 0) {
		    printf("error: failed to read from device #%i\n", num);
		    return -1;
	    }

	    /* write value read to response buffer */
	    _parse_res(dim, &res, data, datalen);
	    memcpy(pdu->payload, data, strlen(data));

            return gcoap_finish(pdu, strlen(data), COAP_FORMAT_TEXT);

        case COAP_PUT:
	case COAP_POST:
	    /* parse payload */
	    printf(" ");  // for the error label statement
	    char payload[10];
	    memcpy(payload, (char *)pdu->payload, pdu->payload_len);
	    uint16_t write_val = (uint16_t)strtoul(payload, NULL, 10);

	    // TODO: write for multiple dimension data
	    /* write to device */
	    phydat_t resp;

	    memset(&resp, 0, sizeof(resp));
	    resp.val[0] = write_val;
            dim = saul_reg_write(dev, &resp);
            if (dim <= 0) {
                if (dim == -ENOTSUP) {
                    printf("error: device #%i is not writable\n", num);
                }
                else {
                    printf("error: failure to write to device #%i\n", num);
                }
                return -1;
            }

	    return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
    }

    return 0;
}

static int _saul_class_to_uri(const char *class, char *uri, size_t urilen, int num)
{
	/* prepend '/' at the start, num at the end*/
	sprintf(uri, "/%i/%s", num, class);

	for (int i = 0; i < (int) strlen(uri); i++) {
		/* replace any '_' with / */
		if (uri[i] == '_')
		    uri[i] = '/';
		else
		    /* change to lowercase */
		    uri[i] = (char)tolower((int)uri[i]);
	}

        if (strlen(uri) > urilen)
            return -1;

	return 0;
}

/*
 * Adds dev to the resource array _resources.
 * Parses the URI from the SAUL_CLASS.
 * Uses _generic_handler for the handler.
 */
int gsc_init(coap_resource_t *resources)
{
    int idx = 0;

    /* FIXME: currently adds all available devices to resources list.
     * How to avoid duplicates?
     */
    saul_reg_t *reg = saul_reg;
    while (reg) {
        /* Parse the SAUL_CLASS for the URI */
        char val_url[NANOCOAP_URL_MAX]; // FIXME: URI longer than '/sense/temp/7' can't be processed
        char td_url[NANOCOAP_URL_MAX];
        const char *class = saul_class_to_str(reg->driver->type);
        _saul_class_to_uri(class, td_url, NANOCOAP_URL_MAX, idx / 2);
        sprintf(val_url, "%s/val", td_url);

        /* Get ops for the device */
        int ops = COAP_GET;
        if (reg->driver->write){
		ops |= COAP_PUT;
		ops |= COAP_POST;
        }

        /* Adds url to list */
	strcpy(_td_urls[idx], td_url);
        strcpy(_val_urls[idx], val_url);

        /* Adds the device to resource list */
	resources[idx].path = _td_urls[idx];
	resources[idx].methods = ops;
	resources[idx].handler = _generic_td_handler;
	resources[idx].context = NULL;

        // FIXME: need something better
	resources[idx + 1].path = _val_urls[idx];
	resources[idx + 1].methods = ops;
	resources[idx + 1].handler = _generic_val_handler;
	resources[idx + 1].context = NULL;

	/* get next reg */
	reg = reg->next;
	idx += 2;
    }
    return 0;
}
