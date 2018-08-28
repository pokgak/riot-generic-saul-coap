#include <stdio.h>
#include <string.h>
#include "net/ipv6.h"
#include "net/gnrc.h"
#include "net/gnrc/netapi.h"

#define TD_CONTEXT "https://w3c.github.io/wot/w3c-wot-td-context.jsonld"
#define GSC_PORT "5683"

int _get_base_url(char *baseurl)
{
    ipv6_addr_t ipv6_addrs[GNRC_NETIF_IPV6_ADDRS_NUMOF];

    /* get iface pid */
    gnrc_netif_t *netif = NULL;

    while ((netif = gnrc_netif_iter(netif))) {
	/* get the all available ipv6 adresses */
	int res = gnrc_netapi_get(netif->pid, NETOPT_IPV6_ADDR, 0, ipv6_addrs,
			sizeof(ipv6_addrs));
	if (res < 0) {
	    /* handle error */
            return -1;
	}
	for (unsigned i = 0; i < (res / sizeof(ipv6_addr_t)); i++) {
            ipv6_addr_to_str(baseurl, ipv6_addrs, IPV6_ADDR_MAX_STR_LEN);
	}
    }

    return 0;
}

const char *_get_type(const char *url)
{
    char type[10];
    char *start = 1 + strchr((const char *)url + 1, '/');
    char *last = strchr((const char *)start + 1, '/');
    snprintf(type, (size_t) (last - start + 1), "%s", start);

    if (strcmp(type, "act") == 0) {
        return "Actuator";
    }
    else if (strcmp(type, "sense") == 0) {
        return "Sensor";
    }
    else {
        return "INVALID_TYPE";
    }
}

const char *_get_name(const char *url)
{
    char name[20];
    char *start = strrchr((const char *) url, '/');
    char *last = (char *) (url + strlen(url));
    snprintf(name, (ssize_t) (last - start), "%s", start + 1);

    /* this can be better */
    if (strcmp(name, "switch") == 0)
        return "Switch";
    else if (strcmp(name, "accel") == 0)
        return "Accelerator";
    else
        return "INVALID_NAME";
}

const char *_is_writable(const char *url)
{
    (void)url;
    return "false";
}

const char *_is_observable(const char *url)
{
    (void)url;
    return "true";
}

const char *_get_href(const char *url)
{
    (void)url;
    return "val";
}

const char *_get_media_type(const char *url)
{
    (void)url;
    return "application/json";
}

ssize_t get_td(char *td, const char *url)
{
    char baseurl[IPV6_ADDR_MAX_STR_LEN];
    _get_base_url(baseurl);

    sprintf(td, "{\n");
    sprintf(td + strlen(td), "  \"@context\": [\"%s\"],\n", TD_CONTEXT);
    sprintf(td + strlen(td), "  \"@type\": [\"%s\"],\n", _get_type(url));
    sprintf(td + strlen(td), "  \"name\": \"%s\",\n", _get_name(url));
    sprintf(td + strlen(td), "  \"base\": \"coap://%s:%s/\",\n", baseurl, GSC_PORT);
    sprintf(td + strlen(td), "  \"interaction\": [\n");
    sprintf(td + strlen(td), "    {\n");
    sprintf(td + strlen(td), "      \"@type\": [\"Property\", \"%s\"],\n", _get_name(url));
    sprintf(td + strlen(td), "      \"schema\": {\n");
    sprintf(td + strlen(td), "        \"type\": \"number\"\n");
    sprintf(td + strlen(td), "      },\n");
    sprintf(td + strlen(td), "      \"writable\": %s,\n", _is_writable(url));
    sprintf(td + strlen(td), "      \"observable\": %s,\n", _is_observable(url));
    sprintf(td + strlen(td), "      \"form\": [{\n");
    sprintf(td + strlen(td), "        \"href\": \"%s\",\n", _get_href(url));
    sprintf(td + strlen(td), "        \"mediaType\": \"%s\"\n", _get_media_type(url));
    sprintf(td + strlen(td), "      }]\n");
    sprintf(td + strlen(td), "    }\n");
    sprintf(td + strlen(td), "  ]\n");
    sprintf(td + strlen(td), "}\n");

    return strlen(td);
}
