#include <stdio.h>
#include <string.h>
#include "net/ipv6.h"
#include "net/gnrc.h"
#include "net/gnrc/netapi.h"
#include "saul_reg.h"

#define TD_CONTEXT "https://w3c.github.io/wot/w3c-wot-td-context.jsonld"
#define GSC_PORT "5683"

extern int get_devnum(const char *url);

int _get_base_url(char *baseurl, size_t len)
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
            ipv6_addr_to_str(baseurl, ipv6_addrs, len);
	}
    }

    /* check for overflow, ipv6_addr_to_str checked this
     * already actually but for safety, why not */
    if (strlen(baseurl) > len)
        return -1;

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
    int devnum = get_devnum(url);
    saul_reg_t *dev = saul_reg_find_nth(devnum);
    if (dev == NULL) {
        puts("_get_name: undefined device number given");
        return "INVALID NAME";
    }

    return dev->name;
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

ssize_t get_td(char *td, size_t tdlen, const char *url)
{
    char baseurl[IPV6_ADDR_MAX_STR_LEN];
    _get_base_url(baseurl, IPV6_ADDR_MAX_STR_LEN);

    snprintf(td, tdlen, "{\n");
    snprintf(td + strlen(td), tdlen, "  \"@context\": [\"%s\"],\n", TD_CONTEXT);
    snprintf(td + strlen(td), tdlen, "  \"@type\": [\"%s\"],\n", _get_type(url));
    snprintf(td + strlen(td), tdlen, "  \"name\": \"%s\",\n", _get_name(url));
    snprintf(td + strlen(td), tdlen, "  \"base\": \"coap://%s:%s/\",\n", baseurl, GSC_PORT);
    snprintf(td + strlen(td), tdlen, "  \"interaction\": [\n");
    snprintf(td + strlen(td), tdlen, "    {\n");
    snprintf(td + strlen(td), tdlen, "      \"@type\": [\"Property\", \"%s\"],\n", _get_name(url));
    snprintf(td + strlen(td), tdlen, "      \"schema\": {\n");
    snprintf(td + strlen(td), tdlen, "        \"type\": \"number\"\n");
    snprintf(td + strlen(td), tdlen, "      },\n");
    snprintf(td + strlen(td), tdlen, "      \"writable\": %s,\n", _is_writable(url));
    snprintf(td + strlen(td), tdlen, "      \"observable\": %s,\n", _is_observable(url));
    snprintf(td + strlen(td), tdlen, "      \"form\": [{\n");
    snprintf(td + strlen(td), tdlen, "        \"href\": \"%s\",\n", _get_href(url));
    snprintf(td + strlen(td), tdlen, "        \"mediaType\": \"%s\"\n", _get_media_type(url));
    snprintf(td + strlen(td), tdlen, "      }]\n");
    snprintf(td + strlen(td), tdlen, "    }\n");
    snprintf(td + strlen(td), tdlen, "  ]\n");
    snprintf(td + strlen(td), tdlen, "}\n");

    size_t td_act_len = strlen(td);
    if (td_act_len > tdlen)
        return -1;

    return td_act_len;
}
