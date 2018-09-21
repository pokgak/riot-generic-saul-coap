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

static cn_cbor *done_td[GSC_MAX_URLS];   /*<Holds all cn_cbor root that already parsed once */

static int _get_base_url(char *baseurl, size_t len)
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

static cn_cbor *_get_properties(const char *url, cn_cbor_context *ct)
{
    cn_cbor *properties = cn_cbor_map_create(ct, NULL);

    /* observable */
    cn_cbor *obs = cn_cbor_data_create(NULL, 0, ct, NULL);
    obs->type = (_is_observable(url) ? CN_CBOR_TRUE : CN_CBOR_FALSE);
    cn_cbor_mapput_string(properties, "observable", obs, ct, NULL);

    /* writable */
    cn_cbor *write = cn_cbor_data_create(NULL, 0, ct, NULL);
    write->type = (_is_writable(url) ? CN_CBOR_TRUE : CN_CBOR_FALSE);
    cn_cbor_mapput_string(properties, "writable", write, ct, NULL);

    return properties;
}

static cn_cbor *_get_actions(const char *url, cn_cbor_context *ct)
{
    (void)url;
    (void)ct;
    return NULL;
}

static cn_cbor *_get_events(const char *url, cn_cbor_context *ct)
{
    (void)url;
    (void)ct;
    return NULL;
}

static cn_cbor *_get_links(const char *url, cn_cbor_context *ct)
{
    (void)url;
    (void)ct;
    _get_media_type(url);
    _get_href(url);
    _get_base_url((char *)url, 0);
    return NULL;
}

static cn_cbor *_get_security(const char *url, cn_cbor_context *ct)
{
    (void)url;
    (void)ct;
    return NULL;
}

/*
 * Returns the Thing Description of the given device/url in cn_cbor struct
 */
cn_cbor *get_td(const char *url, cn_cbor_context *ct)
{
    /* Check if td already parsed */
    int num = get_devnum(url);
    if (done_td[num]) {
        return done_td[num];
    }

    /* if not create new */
    cn_cbor *root = cn_cbor_map_create(ct, NULL);

    /* id */
    cn_cbor_mapput_string(root, "id",
            cn_cbor_string_create(_get_id(url), ct, NULL), ct, NULL);

    /* description */
    const char *desc = _get_desc(url);
    if (desc) {
        cn_cbor_mapput_string(root, "description",
                cn_cbor_string_create(desc, ct, NULL), ct, NULL);
    }

    /* name */
    cn_cbor_mapput_string(root, "name",
            cn_cbor_string_create(_get_name(url), ct, NULL), ct, NULL);

    /* support */
    const char *support = _get_support(url);
    if (support) {
        cn_cbor_mapput_string(root, "support",
                cn_cbor_string_create(support, ct, NULL), ct, NULL);
    }

    /* base */
    const char *base = _get_base(url);
    if (base) {
        cn_cbor_mapput_string(root, "base", 
                cn_cbor_string_create(base, ct, NULL), ct, NULL);
    }

    /* properties */
    cn_cbor *properties = _get_properties(url, ct);
    if (properties) {
        cn_cbor_mapput_string(root, "properties", properties, ct, NULL);
    }

    /* actions */
    cn_cbor *actions = _get_actions(url, ct);
    if (actions) {
        cn_cbor_mapput_string(root, "actions", actions, ct, NULL);
    }

    /* events */
    cn_cbor *events = _get_events(url, ct);
    if (events) {
        cn_cbor_mapput_string(root, "events", events, ct, NULL);
    }

    /* links */
    cn_cbor *links = _get_links(url, ct);
    if (links) {
        cn_cbor_mapput_string(root, "links", links, ct, NULL);
    }

    /* security */
    cn_cbor *security = _get_security(url, ct);
    if (security) {
        cn_cbor_mapput_string(root, "security", security, ct, NULL);
    }

    /* save to done list */
    done_td[num] = root;

    return root;
}
