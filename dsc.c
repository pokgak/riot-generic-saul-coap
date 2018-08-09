#include <stdio.h>
#include "saul_reg.h"
#include "net/gcoap.h"

/* Additional CoAP resources to declare */
static coap_resource_t _resources[10];

static gcoap_listener_t _listener = {
	&_resources[0],
	sizeof(_resources) / sizeof(_resources[0],
	NULL
};

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

    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));

    switch(method_flag) {
        case COAP_GET:
            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);

	    /* get sensor_val from current sensor */
	    phydat_t res;
	    uint16_t dim = saul_reg_read(&dev, &res);

	    /*
	     * sensor values can be more than one dimensions.
	     * Pack all results/dimensions in one line string
	     */
	    char *sensor_val;
	    for (int i = dim; i > 0; i--) {
		sensor_val += res.val[i];
		
		/* count the length at the same time */
		payload_len += sizeof(res.val[i]);
	    }

            /* write the response buffer with the sensor value */
	    sprintf((char *)pdu->payload, "%s", sensor_val);

            return gcoap_finish(pdu, payload_len, COAP_FORMAT_TEXT);

        case COAP_PUT:
	    /* only if sensor allows writing to it */
	    if (writing_allowed) {
	    }
	    else {
                return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST); // TODO: change COAP_CODE
	    }

            /* convert the payload to an integer and update the internal
               value */
            if (pdu->payload_len <= 5) {
                char payload[6] = { 0 };
                memcpy(payload, (char *)pdu->payload, pdu->payload_len);
                req_count = (uint16_t)strtoul(payload, NULL, 10);
                return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
            }
            else {
                return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
            }
    }

    return 0;
}

/*
 * Adds dev to the resource array _resources.
 * Parses the URI from the SAUL_CLASS.
 * Uses _generic_handler for the handler.
 */
static int _add_resource(saul_reg_t *dev, int idx)
{
    /* Parse the SAUL_CLASS for the URI */
    const char uri = *((const char *) _saul_class_to_uri(saul_class_to_str(dev->driver->type)));

    /* Get ops for the device */
    int ops = COAP_GET;
    if (dev->driver->write){
	    ops |= COAP_PUT; /* TODO: how about COAP_POST? */
    }

    /* Adds the device to resource list */
    _resources[idx] = { uri, ops, _generic_handler, NULL };

    return 0;
}

static char *_saul_class_to_uri(char *class)
{
	char *uri;
	uint8_t i = 0;

	/* prepend '/' at the start */
	sprintf(uri, "/%s", *((const char *) class));

	for (c = uri[i]; uri[i]; ++i, c = uri[i]) {
		/* change to lowercase */
		uri[i] = tolower(c);

		/* replace any '_' with / */
		if (uri[i] == '_')
			uri[i] = '/';
	}

	return uri;
}

/*
 * Finds devices available and adds it to the resources list
 */
static void find_devices(void)
{
    int idx = 0;

    saul_reg_t reg = saul_reg;
    while (reg->next) {
        saul_reg_t dev = reg->next;
	add_resource(&dev, idx);
    }
}

void dsc_init(void)
{
    find_devices();
    gcoap_register_listener(&_listener);
}
