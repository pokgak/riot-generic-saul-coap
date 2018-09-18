#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "net/ipv6.h"
#include "net/gnrc.h"
#include "net/gnrc/netapi.h"
#include "saul_reg.h"
#include "gsc.h"
#include "cn-cbor/cn-cbor.h"

#define TD_CONTEXT "https://w3c.github.io/wot/w3c-wot-td-context.jsonld"
#define GSC_PORT "5683"

extern int get_devnum(const char *url);
extern const char *get_type(const char *url);

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

static const char *_get_name(const char *url)
{
    int devnum = get_devnum(url);
    saul_reg_t *dev = saul_reg_find_nth(devnum);
    if (dev == NULL) {
        puts("_get_name: undefined device number given");
        return "INVALID NAME";
    }

    return dev->name;
}

static bool _is_writable(const char *url)
{
    (void)url;
    return false;
}

static bool _is_observable(const char *url)
{
    (void)url;
    return true;
}

static const char *_get_href(const char *url)
{
    (void)url;
    return "val";
}

static const char *_get_media_type(const char *url)
{
    (void)url;
    return "application/json";
}

static const char *_get_id(const char *url)
{
    (void)url;
    return "ID_EXAMPLE";
}

static const char *_get_desc(const char *url)
{
    (void)url;
    return "enter description here";
}

static const char *_get_support(const char *url)
{
    (void)url;
    return NULL;
}

static const char *_get_base(const char *url)
{
    (void)url;
    return NULL;
}

static cn_cbor *_get_properties(const char *url)
{
    cn_cbor *properties = cn_cbor_map_create(NULL);

    /* observable */
    cn_cbor *obs = cn_cbor_data_create(NULL, 0, NULL);
    obs->type = (_is_observable(url) ? CN_CBOR_TRUE : CN_CBOR_FALSE);
    cn_cbor_mapput_string(properties, "observable", obs, NULL);

    /* writable */
    cn_cbor *write = cn_cbor_data_create(NULL, 0, NULL);
    write->type = (_is_writable(url) ? CN_CBOR_TRUE : CN_CBOR_FALSE);
    cn_cbor_mapput_string(properties, "writable", write, NULL);

    return properties;
}

static cn_cbor *_get_actions(const char *url)
{
    (void)url;
    return NULL;
}

static cn_cbor *_get_events(const char *url)
{
    (void)url;
    return NULL;
}

static cn_cbor *_get_links(const char *url)
{
    (void)url;
    _get_media_type(url);
    _get_href(url);
    _get_base_url((char *)url, 0);
    return NULL;
}

static cn_cbor *_get_security(const char *url)
{
    (void)url;
    return NULL;
}

/*
 * Returns the Thing Description of the given device/url in cn_cbor struct
 */
cn_cbor *get_td(const char *url)
{
    cn_cbor *root = cn_cbor_map_create(NULL);

    /* id */
    cn_cbor_mapput_string(root, "id",
            cn_cbor_string_create(_get_id(url), NULL), NULL);

    /* description */
    const char *desc = _get_desc(url);
    if (desc) {
        cn_cbor_mapput_string(root, "description",
                cn_cbor_string_create(desc, NULL), NULL);
    }

    /* name */
    cn_cbor_mapput_string(root, "name",
            cn_cbor_string_create(_get_name(url), NULL), NULL);

    /* support */
    const char *support = _get_support(url);
    if (support) {
        cn_cbor_mapput_string(root, "support",
                cn_cbor_string_create(support, NULL), NULL);
    }

    /* base */
    const char *base = _get_base(url);
    if (base) {
        cn_cbor_mapput_string(root, "base", 
                cn_cbor_string_create(base, NULL), NULL);
    }

    /* properties */
    cn_cbor *properties = _get_properties(url);
    if (properties) {
        cn_cbor_mapput_string(root, "properties", properties, NULL);
    }

    /* actions */
    cn_cbor *actions = _get_actions(url);
    if (actions) {
        cn_cbor_mapput_string(root, "actions", actions, NULL);
    }

    /* events */
    cn_cbor *events = _get_events(url);
    if (events) {
        cn_cbor_mapput_string(root, "events", events, NULL);
    }

    /* links */
    cn_cbor *links = _get_links(url);
    if (links) {
        cn_cbor_mapput_string(root, "links", links, NULL);
    }

    /* security */
    cn_cbor *security = _get_security(url);
    if (security) {
        cn_cbor_mapput_string(root, "security", security, NULL);
    }

    return root;
}
