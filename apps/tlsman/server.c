#include <stdio.h>
#include <inttypes.h>

#include "net/sock/udp.h"
#include "net/tlsman.h"

#include "msg.h" /* TODO */

#define ENABLE_DEBUG (1)
#include "debug.h"

#ifndef DTLS_DEFAULT_PORT
#define DTLS_DEFAULT_PORT 20220 /* DTLS default port */
#endif

#define READER_QUEUE_SIZE (8U)

static void _resp_handler(uint8_t *data, size_t data_size, void *sock);

static tlsman_session_t dtls_session;

static void _resp_handler(uint8_t *data, size_t data_size, void *sock)
{

    (void) sock;

    printf("Got (string) data -- ");
    for (size_t i = 0; i < data_size; i++) {
        printf("%c", data[i]);
    }
    printf(" -- (Total: %i bytes)\n",data_size );

    /* TODO FROM ... */
    /* NOTE: Remote is already in dtls_session */
    tlsman_send_data_app(&dtls_session, "Pong", sizeof("Pong"));
}

void _server_wrapper(void *arg) {
    (void) arg;

    ssize_t pckt_size = DTLS_MAX_BUF;
    uint8_t pckt_rcvd[DTLS_MAX_BUF];

    sock_udp_t udp_sock;
    sock_udp_ep_t local = SOCK_IPV6_EP_ANY;
    sock_udp_ep_t remote = SOCK_IPV6_EP_ANY;

    uint8_t tlsman_flags =  TLSMAN_FLAG_STACK_UNIVERSAL |
                            TLSMAN_FLAG_SIDE_SERVER;

    /* TODO Prepare (thread) messages reception */

    local.port = DTLS_DEFAULT_PORT;
    if (sock_udp_create(&udp_sock, &local, NULL, 0) < 0) {
        puts("ERROR: Unable create sock.");
        return;
    }

    ssize_t res = tlsman_init_context((tlsman_ep_t *) &local,
                                     (tlsman_ep_t *) &remote,
                                    &dtls_session, &udp_sock,
                                    _resp_handler, tlsman_flags);

    if (res != 0) {
        puts("ERROR: Unable to init tlsman context!");
        return;
    }

#if ENABLE_DEBUG
    ipv6_addr_t addrs[GNRC_NETIF_IPV6_ADDRS_NUMOF];
    gnrc_netif_t *netif = gnrc_netif_iter(NULL);
    if ((netif != NULL) &&
        ((res = gnrc_netif_ipv6_addrs_get(netif, addrs, sizeof(addrs))) > 0)) {
        printf("Listening (D)TLS request in: ");
        for (unsigned i = 0; i < (res / sizeof(ipv6_addr_t)); i++) {
            printf("[");
            ipv6_addr_print(&addrs[i]);
            printf("]:%u\n", DTLS_DEFAULT_PORT);
        }
    }
#endif

    while(tlsman_listening(&dtls_session, tlsman_flags,
                            pckt_rcvd, DTLS_MAX_BUF)) {

        (void) pckt_size; /* FIXME */
        xtimer_usleep(500);
    }

    return;
}

int server_cmd(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    /* TODO Upgrade this */
    _server_wrapper(NULL);
    return 0;
}
