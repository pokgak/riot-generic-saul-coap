#include <stdio.h>
#include <ctype.h>
#include "net/gcoap.h"
#include "fmt.h"
#include "saul_reg.h"

/*
 * Struct pairing to hold corresponding pair of URI
 * with the device together
 * FIXME: tis the right way to use structs?
 */
typedef struct {
    char url[NANOCOAP_URL_MAX];		/**< URL of device */
    uint8_t num;		/**< Corresponding device */
} saul_coap_t;

/* Pairings of URI and devices */
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
 * Generic handler for the resources. Accepts either a GET or a PUT.
 * Ideally, this can be extended for specific task.
 * Only read or write values for now.
 */
static ssize_t _generic_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;


    /* find which sensor we are currently dealing with */
    saul_reg_t *dev;
    int num = _get_devnum((const char *)pdu->url);
    if (num < 0) {
	    printf("_generic_handler: cannot find dev with URI %s\n", pdu->url);
	    return -ENODEV;
    }

    dev = saul_reg_find_nth(num);
    if (dev == NULL) {
        puts("error: undefined device id (num) given");
        return -ENODEV;
    }

    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));

    switch(method_flag) {
        case COAP_GET:
            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
	    
	    /* read from the device */
	    int dim;
	    phydat_t res;

	    dim = saul_reg_read(dev, &res);
	    if (dim <= 0) {
		    printf("error: failed to read from device #%i\n", num);
		    return -1;
	    }

	    // TODO: Handling of devices with triple dimension data value
	    /* write value read to response buffer */
	    char read_val[50];
	    sprintf(read_val, "%d %s", res.val[0], phydat_unit_to_str(res.unit));
	    memcpy(pdu->payload, read_val, strlen(read_val));

            return gcoap_finish(pdu, strlen(read_val), COAP_FORMAT_TEXT);

        case COAP_PUT:
	    /* TODO */
	    break;
    }

    return 0;
}

static int _saul_class_to_uri(const char *class, char *uri, int num)
{
	/* prepend '/' at the start */
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

    printf("url: %s\n", url);

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

    puts("Init dsc");
    _add_devs_to_resources();
    gcoap_register_listener(&_listener);
    puts("finish init dsc");

    return 0;
}
