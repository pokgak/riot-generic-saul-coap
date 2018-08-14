#include <stdio.h>
#include <ctype.h>
#include "net/gcoap.h"
#include "fmt.h"
#include "saul_reg.h"

/*
 * Struct pairing to hold corresponding pair of URI
 * with the device num together
 */
typedef struct {
    char url[NANOCOAP_URL_MAX];		/**< URL of device */
    uint8_t num;			/**< Corresponding device */
} saul_coap_t;

/* Pairings of URI and device numbers */
static saul_coap_t _pairs[15];

/* Additional CoAP resources to declare */
static coap_resource_t _resources[15];

static gcoap_listener_t _listener = {
    &_resources[0],
    sizeof(_resources) / sizeof(_resources[0]),
    NULL
};

/*
 * Parses URL for device num. Used to retrieve
 * device from registry
 */
static int _get_devnum(const char *url)
{
	char *start = strrchr(url, '/');	
	if (start)
		return atoi((const char *) (start + 1));
	else
		return -ENODEV;
}

/*
 * Parse the (multidimensional) result res from given phydat_t and
 * write the value to the char *data
 */
static ssize_t _parse_res(uint8_t dim, phydat_t *res, char *data)
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

    return 0;
}

/*
 * Generic handler for the resources. Accepts either a GET or a PUT.
 * Only read or write values for now.
 */
static ssize_t _generic_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, void *ctx)
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

	    // TODO: Handling of devices with triple dimension data value
	    /* write value read to response buffer */
	    char data[50];
	    _parse_res(dim, &res, data);
	    memcpy(pdu->payload, data, strlen(data));

            return gcoap_finish(pdu, strlen(data), COAP_FORMAT_TEXT);

        case COAP_PUT:
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

static int _saul_class_to_uri(const char *class, char *uri, int num)
{
	/* prepend '/' at the start, num at the end*/
	sprintf(uri, "/%s/%i", class, num);

	for (uint8_t i = 0; i < strlen(uri); i++) {
		/* replace any '_' with / */
		if (uri[i] == '_')
		    uri[i] = '/';
		else
		    /* change to lowercase */
		    uri[i] = (char)tolower((int)uri[i]);
	}

	return 0;
}

/*
 * Adds dev to the resource array _resources.
 * Parses the URI from the SAUL_CLASS.
 * Uses _generic_handler for the handler.
 *
 * TODO: Change idx to something better
 */
static int _add_resource(saul_reg_t *dev, int idx)
{
    /* Parse the SAUL_CLASS for the URI */
    char url[NANOCOAP_URL_MAX];
    const char *class = saul_class_to_str(dev->driver->type);
    _saul_class_to_uri(class, url, idx);

    /* Get ops for the device */
    int ops = COAP_GET;
    if (dev->driver->write){
	    ops |= COAP_PUT; /* TODO: how about COAP_POST? */
    }

    /* TODO: standardise assignment of structs */
    /* Adds to pairing list */
    saul_coap_t pair;
    strcpy(pair.url, url);
    pair.num = idx;
    _pairs[idx] = pair;

    /* Adds the device to resource list */
    coap_resource_t rsc = {
	    .path = _pairs[idx].url,
	    .methods = ops,
	    .handler = _generic_handler,
	    .context = NULL
    };
    _resources[idx] = rsc;

    return 0;
}

/*
 * Finds devices available and adds it to the resources list
 */
static void _add_devs_to_resources(void)
{
    int idx = 0;

    /* FIXME: currently adds all available devices to resources list.
     * How to avoid duplicates?
     * FIXME: resources must be sorted alphabetically based on resource paths 
     */
    saul_reg_t *reg = saul_reg;
    while (reg) {
	_add_resource(reg, idx);
	reg = reg->next;
	idx++;
    }

}

int dsc_init(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    _add_devs_to_resources();
    gcoap_register_listener(&_listener);

    return 0;
}
