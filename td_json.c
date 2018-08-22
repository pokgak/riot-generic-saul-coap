#include <stdio.h>
#include <string.h>
#include "net/ipv6.h"
#include "net/gnrc.h"
#include "net/gnrc/netapi.h"

#define TD_CONTEXT "https://w3c.github.io/wot/w3c-wot-td-context.jsonld"
#define PORT "5683"

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
            printf("baseurl: %s\n", baseurl);
	}
    }

    return 0;
}

const char *_get_type(char *url)
{
    char *start = url + 1;
    char *last = strchr((const char *)start, '/');
    char type[10];
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

const char *_get_name(char *url)
{
    char *start = strchr((const char *) (url + 1), '/');
    char *last = strrchr((const char *) url, '/');
    char name[20];
    snprintf(name, (size_t) (last - start), "%s", start + 1);

    /* this can be better */
    if (strcmp(name, "switch") == 0)
        return "Switch";
    else if (strcmp(name, "accel") == 0)
        return "Accelerator";
    else
        return "INVALID_NAME";
}

int get_td(char *url)
{
    char td[256];
    char baseurl[IPV6_ADDR_MAX_STR_LEN];

    sprintf(td, "{\n");
    sprintf(td + strlen(td), "\t\"@context\": [\"%s\"],\n", TD_CONTEXT);
    sprintf(td + strlen(td), "\t\"@type\": [\"%s\"],\n", _get_type(url));
    sprintf(td + strlen(td), "\t\"name\": \"%s\",\n", _get_name(url));
    sprintf(td + strlen(td), "\t\"base\": \"coap://%s:%s/\",\n", baseurl, PORT);

    printf("%s", td);

    return 0;
}
