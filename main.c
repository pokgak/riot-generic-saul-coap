#include <stdio.h>
#include "msg.h"

#include "net/gcoap.h"
#include "kernel_types.h"
#include "shell.h"

#define MAIN_QUEUE_SIZE (4)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

/* Additional CoAP resources to declare */
static coap_resource_t _resources[15 * 2]; /* for td and val */

extern int gcoap_cli_cmd(int argc, char **argv);
extern void gcoap_cli_init(void);
extern int gsc_init(coap_resource_t *resources); 
extern int get_td(char *out, size_t len, char *url);
extern int get_devnum(const char *url);

extern int _get_base_url(char *baseurl, size_t len);
extern const char *_get_type(char *url);
extern const char *_get_name(char *url);

static gcoap_listener_t _listener = {
    &_resources[0],
    sizeof(_resources) / sizeof(_resources[0]),
    NULL
};

int test_td(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    char baseurl[IPV6_ADDR_MAX_STR_LEN];
    _get_base_url(baseurl, IPV6_ADDR_MAX_STR_LEN);

    char *url = "/0/sense/switch";
    const char *type = _get_type(url);
    printf("type: %s\n", type);

    const char *name = _get_name(url);
    printf("name: %s\n", name);

    size_t tdlen = 512;
    char td[tdlen];
    get_td(td, tdlen, url);
    printf("td:\n%s\n", td);

    return 0;
}

int test_gsc(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    const char *url = "/0/sense/switch";
    get_devnum(url);

    return 0;
}

int gsc_handler(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    gsc_init(_resources);
    gcoap_register_listener(&_listener);

    return 0;
}

static const shell_command_t shell_commands[] = {
    { "coap", "CoAP example", gcoap_cli_cmd },
    { "gsc", "init gsc", gsc_handler },
    { "test_td", "test TD components", test_td },
    { "test_gsc", "test gsc", test_gsc },
    { NULL, NULL, NULL }
};

int main(void)
{
    /* for the thread running the shell */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    gcoap_cli_init();
    puts("gcoap example app");

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should never be reached */
    return 0;
}
