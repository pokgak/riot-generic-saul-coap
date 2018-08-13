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
    const char *url;		/**< URL of device */
    saul_reg_t *dev;		/**< Corresponding device */
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
 * Parses the URI to get the corresponding device
 */
static int _get_dev_from_uri(char *url, saul_reg_t *dev)
{
    /* Get the device from pair list*/
    for (int i = 0; i < (int)(sizeof(_resources) / sizeof(saul_coap_t)); i++) {
	if (!strcmp(url, (const char *)_pairs[i].url)) {
		dev = _pairs[i].dev;
		return 0;
	}
    }
    (void)dev;

    return -1; /* dev not found, error */
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
    saul_reg_t dev;
    int ret = _get_dev_from_uri((char *)pdu->url, &dev);
    if (ret != 0) {
	    puts("_generic_handler: dev not found");
	    return -1;
    }

    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));

    switch(method_flag) {
        case COAP_GET:
            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
	    
	    /* Just returns 0 as dummy for now */
	    size_t payload_len = fmt_u16_dec((char *)pdu->payload, 0);

            return gcoap_finish(pdu, payload_len, COAP_FORMAT_TEXT);

        case COAP_PUT:
	    /* TODO */
	    break;
    }

    return 0;
}

static int _saul_class_to_uri(const char *class, char *uri)
{
	/* prepend '/' at the start */
	sprintf(uri, "/%s", class);


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
    _saul_class_to_uri(saul_class_to_str(dev->driver->type), url);

    printf("url: %s\n", url);

    /* Get ops for the device */
    int ops = COAP_GET;
    if (dev->driver->write){
	    ops |= COAP_PUT; /* TODO: how about COAP_POST? */
    }

    /* Adds the device to resource list */
    coap_resource_t rsc = {
	    .path = (const char *)url,
	    .methods = ops,
	    .handler = _generic_handler,
	    .context = NULL
    };
    _resources[idx] = rsc;

    /* Adds to pairing list */
    saul_coap_t pair = {
	.url = (const char *)url,
	.dev = dev
    };
    _pairs[idx] = pair;

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
