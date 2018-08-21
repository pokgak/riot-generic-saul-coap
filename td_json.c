#include <stdio.h>
#include <string.h>
#include "net/ipv6.h"
#include "net/gnrc.h"
#include "net/gnrc/netapi.h"

int _get_base_url(void)
{
    char addr_str[IPV6_ADDR_MAX_STR_LEN];
    ipv6_addr_t ipv6_addrs[GNRC_NETIF_IPV6_ADDRS_NUMOF];

    /* get iface pid */
    gnrc_netif_t *netif = NULL;

    while ((netif = gnrc_netif_iter(netif))) {
	/* get the all available ipv6 adresses */
	int res = gnrc_netapi_get(netif->pid, NETOPT_IPV6_ADDR, 0, ipv6_addrs,
			sizeof(ipv6_addrs));
	if (res) {
	    /* handle error */
	}
	for (unsigned i = 0; i < (res / sizeof(ipv6_addr_t)); i++) {
            ipv6_addr_to_str(addr_str, ipv6_addrs, sizeof(addr_str));
            printf("baseurl: %s\n", addr_str);
	}
    }


    return 0;
}
