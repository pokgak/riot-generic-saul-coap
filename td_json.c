#include <stdio.h>
#include <string.h>
#include "net/ipv6.h"
#include "net/gnrc.h"
#include "net/gnrc/netapi.h"

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
    printf("type: %s\n", type);

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
