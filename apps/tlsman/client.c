
 #include <stdio.h>

#include "net/sock/udp.h"
#include "net/tlsman.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

/* Our custom response handler (optional) */
static void _resp_handler(uint8_t *data, size_t data_size, void *sock);

static void _resp_handler(uint8_t *data, size_t data_size, void *sock)
{

    (void) sock;
    printf("Answer (%i bytes): \t--%s--\n", data_size, data);
}

int _client_side(char *addr_str,  uint16_t  port)
{
    sock_udp_t udp_sock;
    sock_udp_ep_t local = SOCK_IPV6_EP_ANY;
    sock_udp_ep_t remote = SOCK_IPV6_EP_ANY;
    uint8_t tlsman_flags =  TLSMAN_FLAG_STACK_UNIVERSAL |
                            TLSMAN_FLAG_SIDE_CLIENT;

    tlsman_session_t dtls_session;
    uint8_t packet_rcvd[DTLS_MAX_BUF];

    DEBUG("Remote server: [%s]:%u\n", addr_str, port);

    local.port = (unsigned short) port + 10;
    remote.port = (unsigned short) port;
    ipv6_addr_from_str((ipv6_addr_t *)&remote.addr.ipv6, addr_str);
    sock_udp_create(&udp_sock, &local, &remote, 0);

    ssize_t res = tlsman_init_context((tlsman_ep_t *) &local,
                                     (tlsman_ep_t *) &remote,
                                    &dtls_session, &udp_sock,
                                    _resp_handler, tlsman_flags);

    if (res != 0) {
        puts("ERROR: Unable to init tlsman context!");
        return -1;
    }

    res = tlsman_create_channel(&dtls_session, tlsman_flags,
                          packet_rcvd, DTLS_MAX_BUF);

    if (tlsman_process_is_error_code_nonfatal(res)) {
        puts("ERROR: Unable to start (D)TLS handhsake process!");
        return -1;
    }
    else if (res == TLSMAN_ERROR_HANDSHAKE_TIMEOUT) {
        /* NOTE: Handhsake timeout can be not fatal but is part of our test */
        puts("ERROR: (D)TLS handshake timeout!");
    }

    while(tlsman_is_channel_ready(&dtls_session)) {
        /* TODO: Add  user control */
        printf("Send (%i bytes): \t--%s--\n", sizeof("Ping"), "Ping");
        tlsman_send_data_app(&dtls_session, "Ping", sizeof("Ping"));
        xtimer_usleep(100); /* Simulating other operations */
        tlsman_retrieve_data_app(&dtls_session, packet_rcvd, DTLS_MAX_BUF);
        xtimer_sleep(5);
    }

    return 0;
}


int client_cmd(int argc, char **argv)
{

    if ((argc != 3)) {
        printf("usage: %s <IPv6 Address> <(UDP|TCP) Port>\n", argv[0]);
        return 1;
    }

    /* TODO Add safe guard with the atoi */

    return _client_side(argv[1], atoi(argv[2]) );
}
