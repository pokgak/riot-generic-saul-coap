#include <stdio.h>
#include "net/gcoap.h"
#include "fmt.h"
#include "saul_reg.h"
#include "gsc.h"

extern ssize_t get_td(char *out, size_t tdlen, const char *url);

static gsc_t gsc_devs[GSC_MAX_URLS];

/*
 * Parses URL for device num. Used to retrieve device from saul registry
 */
int get_devnum(const char *url)
{
    return atoi((const char *) url + 1);
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
    snprintf(data, datalen, "Data:");
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

        snprintf(data + strlen(data), datalen, "\t");
        if (dim > 1) {
            snprintf(data + strlen(data), datalen, "[%u] ", (unsigned int)i);
        }
        else {
            snprintf(data + strlen(data), datalen, "     ");
        }
        if (scale_prefix) {
            snprintf(data + strlen(data), datalen, "%6d %c", (int)res->val[i], scale_prefix);
        }
        else if (res->scale == 0) {
            snprintf(data + strlen(data), datalen, "%6d", (int)res->val[i]);
        }
        else if ((res->scale > -5) && (res->scale < 0)) {
            char num[8];
            size_t len = fmt_s16_dfp(num, res->val[i], res->scale);
            num[len] = '\0';
            snprintf(data + strlen(data), datalen, "%s", num);
        }
        else {
            snprintf(data + strlen(data), datalen, "%iE%i", (int)res->val[i], (int)res->scale);
        }

        snprintf(data + strlen(data), datalen, "%s\n", phydat_unit_to_str(res->unit));
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

    // FIXME: without this printf _get_type in get_td will return INVALID TYPE
    size_t tdlen = 512;
    char td[tdlen];
    size_t td_len = get_td(td, tdlen, (const char *)(pdu->url));
    gcoap_resp_init(pdu, buf,  len, COAP_CODE_CONTENT);
    memcpy(pdu->payload, td, td_len);

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
    int num = get_devnum((const char *)pdu->url);
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

const char *get_type(const char *url)
{
    return gsc_devs[get_devnum(url)].type;
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
    while (reg && 
	  (idx < GSC_MAX_URLS)) {
        /* set gsc_t devno */
        gsc_devs[idx].devno = idx;

        /* set gsc_t type */
        char *start = (char *)saul_class_to_str(reg->driver->type);
        char *last = strchr(start + 1, '_');
        char type[15];
        snprintf(type, last - start + 1, "%s", start);

        if (strcmp(type, "ACT") == 0)
            gsc_devs[idx].type = "Actuator";
        else if (strcmp(type, "SENSE") == 0)
            gsc_devs[idx].type = "Sensor";
        else
            gsc_devs[idx].type = "INVALID_TYPE";

        /* create td and val url */
        char val_url[NANOCOAP_URL_MAX];
        char td_url[NANOCOAP_URL_MAX];
        snprintf(td_url, NANOCOAP_URL_MAX, "/%d", idx);
        snprintf(val_url, NANOCOAP_URL_MAX, "%s/val", td_url);

        /* Adds url to list */
	strcpy(gsc_devs[idx].td_url, td_url);
        strcpy(gsc_devs[idx].val_url, val_url);

        /* Get ops for the device */
        int ops = COAP_GET;
        if (reg->driver->write){
		ops |= COAP_PUT;
		ops |= COAP_POST;
        }

        /* Adds the device to resource list */
	int pos = idx * 2;
	resources[pos].path = gsc_devs[idx].td_url;
	resources[pos].methods = ops;
	resources[pos].handler = _generic_td_handler;
	resources[pos].context = NULL;

	resources[pos + 1].path = gsc_devs[idx].val_url;
	resources[pos + 1].methods = ops;
	resources[pos + 1].handler = _generic_val_handler;
	resources[pos + 1].context = NULL;

	/* get next reg */
	reg = reg->next;
	idx++;
    }
    return 0;
}
