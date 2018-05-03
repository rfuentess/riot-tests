/*
 * Copyright (C) 2018 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       UDP sock asynchronous client side
 *
 * @author      Raul Fuentes <raul.fuentes-samaniego@inria.fr>
 *
 * @}
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <net/ipv6/addr.h>

#include "net/sock/udp.h"

#ifdef SOCK_HAS_ASYNC
#include "event/timeout.h"
#endif

#define ENABLE_DEBUG (1)
#include "debug.h"

#ifndef SERVER_DEFAULT_PORT
#define SERVER_DEFAULT_PORT 20220
#endif

#define CLIENT_PORT SERVER_DEFAULT_PORT + 1
#define MAX_TIMES_TRY_TO_SEND 10

/* TODO: Global (D)TLS session(s)? */

/* TODO: Handling importation of keys */

/* TODO: TLS/DTLS events state */

/* WARNING FIXME Remove this line (once DTLS is integrated)*/
#ifndef DTLS_MAX_BUF
#define DTLS_MAX_BUF 140
#endif


#ifdef SOCK_HAS_ASYNC

/*
 * This is the "client" part for an asynchronous client.
 *
 * WARNING: Not fully tested
 *
 */
#define SOCK_EVENT_KILL_THREAD   (0x00000002)

#define MAX_RETRANSMISON_BEFORE_TIMEOUT 2
#define SERVER_ANSWER_TIMEOUT    (5U * US_PER_SEC)

#define CLIENT_QUEUE_SIZE (8U)
static kernel_pid_t _client_pid = KERNEL_PID_UNDEF;
char _client_stack[THREAD_STACKSIZE_MAIN + THREAD_EXTRA_STACKSIZE_PRINTF];

static uint8_t pckt_rcvd[DTLS_MAX_BUF];
static void _asyn_recv(event_t *event);
static void _kill_thread(event_t *event);
static event_t kill_thread_event = { .handler = _kill_thread };
static event_timeout_t kill_thread_timeout;
static bool _client_listen = true;

static uint8_t _asyn_recv_watchdog;

typedef struct {
    char *addr;
    char *data;
    unsigned int *delay;
} client_args_t;

/*
 * To be used to determine if timeout for the server happened
 */
static void _kill_thread(event_t *event){

    (void) event;
    DEBUG("(client) DBG: event _kill_thread (%u)\n", _asyn_recv_watchdog - 1);

    if (--_asyn_recv_watchdog){
        event_timeout_set(&kill_thread_timeout, SERVER_ANSWER_TIMEOUT);
    } else {
        DEBUG("\t\tTimeout!\n");
        _client_listen = false; /* FIXME: Mutex operation? */
    }
}

static void _asyn_recv(event_t *arg)
{
    /* NOTE: Not fully tested  */

    sock_event_t *event = (sock_event_t *)arg;

    DEBUG("(client) DBG: Event received! (type 0x%08x) \n", event->type);

    /* TODO: Generate our own TLS_APP_DATA_RECV */
    if (event->type & SOCK_EVENT_RECV) {
        sock_udp_ep_t remote = { .family = AF_INET6 };
        ssize_t res;

          while (( res = sock_udp_recv(event->sock, pckt_rcvd, sizeof(pckt_rcvd),
                            0, &remote)) >= 0) {
          if (res >= 0) {
            printf("(client) ACK received -- ");
            ssize_t i;
            for (i = 0; i < DTLS_MAX_BUF; i++) {
                printf("%c", pckt_rcvd[i]);
            }
            printf(" ---\n");

            /* Are there more messages in the near future?  */
            _asyn_recv_watchdog++;

            /* NOTE: enable for sending packets forever */
            /* sock_udp_send(event->sock, "PING", 5, &remote ); */

          } else {
            /* TODO */
          }
        } /* while */
    }
}


/*
 * Llaunch two events and send the first sock message.
 *
 * One event will be for determine if an timeout happened.
 * Second event will be for receiving (async) SOCK messages
 *
 * NOTE: The timeout event has more sense for handshakes process (e.g. dtls)
 */
void *_client_wrapper(void *arg){

    event_t *event;
    client_args_t * _usr_args  = (client_args_t *) arg;
    uint32_t  delay = (uint32_t ) *_usr_args->delay ;

    /* BUG Must be static otherwise failed assertation (event.c#26) */
    static sock_udp_t udp_sock;
    event_queue_t sock_event_queue;

    sock_udp_ep_t remote = SOCK_IPV6_EP_ANY;
    sock_udp_ep_t local = SOCK_IPV6_EP_ANY;

    ssize_t app_data_buf = 0; /* The size of the Upper layer packet to send */

    DEBUG("(client) DBG: Asynchronous mode enabled\n");

    _asyn_recv_watchdog = MAX_RETRANSMISON_BEFORE_TIMEOUT;

    local.port = (unsigned short) CLIENT_PORT;
    remote.port = (unsigned short) SERVER_DEFAULT_PORT;

    /* Parse <IPv6-address>[%netif] */
    int iface = ipv6_addr_split_iface(_usr_args->addr);
    if (iface == -1) {
        if (gnrc_netif_numof() == 1) {
            /* assign the single interface found in gnrc_netif_numof() */
            remote.netif = (uint16_t)gnrc_netif_iter(NULL)->pid;
        }
        else {
            remote.netif = SOCK_ADDR_ANY_NETIF;
        }
    }
    else {
        if (gnrc_netif_get_by_pid(iface) == NULL) {
            puts("(client) ERROR: interface not valid");
            return (void *) NULL;
        }
        remote.netif = iface;
    }

    if (ipv6_addr_from_str( (ipv6_addr_t *)remote.addr.ipv6, _usr_args->addr) == NULL) {
        puts("(client) Error: unable to parse destination address");
        return (void *) NULL;
    }

    app_data_buf = strlen(_usr_args->data);

    sock_udp_create(&udp_sock, &local, &remote, 0);

    /* We set the async elements */
    event_queue_init(&sock_event_queue);
    event_timeout_init(&kill_thread_timeout, &sock_event_queue,
                       &kill_thread_event);

    sock_udp_set_event_queue(&udp_sock, &sock_event_queue, _asyn_recv);
    event_timeout_set(&kill_thread_timeout, SERVER_ANSWER_TIMEOUT);

    sock_udp_send(&udp_sock, _usr_args->data, app_data_buf, &remote );
    while (_client_listen){
      event = event_get(&sock_event_queue);
      if (event){
          event->handler(event);
          xtimer_usleep(delay);  /* FIXME: Magic number for now */
      }
    }

    event_timeout_clear(&kill_thread_timeout);
    sock_udp_set_event_queue(&udp_sock, NULL, NULL);
    sock_udp_close(&udp_sock);

    DEBUG("(client) DBG: Asynchronous mode finished\n");

    return (void *) NULL;
}

/*
 *  This can be seen as the equivalent of _start_server as will
 *  launch a new thread for using sock (UDP or SECURE)-
 *  However, this new thread will terminate by itself.
 *
 */
static void _start_asyn_thread(char *addr_str, char *data,
                        unsigned int delay){

    (void) addr_str;
    (void) data;
    (void) delay;

    client_args_t args = { .addr = addr_str, .data = data, .delay = &delay };

    DEBUG("Parsing: %s - %s - %u\n", args.addr, args.data, *args.delay);

    /* TODO: Improve this! (send a queue?) */
    if (_client_pid != KERNEL_PID_UNDEF) {
      int res = thread_getstatus(_client_pid);

      DEBUG("(client) DBG: Status of previous thread: %i\n", res);

      if (!((res == STATUS_STOPPED) || (res == STATUS_NOT_FOUND ))){
        puts("(client) Error: Previous client is still running");
        return;
        }
    }

    _client_pid = thread_create(_client_stack, sizeof(_client_stack),
                                     THREAD_PRIORITY_MAIN - 1,
                                     THREAD_CREATE_STACKTEST,
                                     _client_wrapper,
                                     &args,
                                     "sock_client");

    DEBUG("(client) DBG: Status of new thread: %i\n", thread_getstatus(_client_pid));

    /* Uncommon but better be sure */
    if (_client_pid == EINVAL) {
        puts("(client) ERROR: Thread invalid");
        _client_pid = KERNEL_PID_UNDEF;
        return;
    }

    if (_client_pid == EOVERFLOW) {
        puts("(client) ERROR: Thread overflow!");
        _client_pid = KERNEL_PID_UNDEF;
        return;
    }

    return;
}

#else /* SOCK_HAS_ASYNC */
/*
 * This is the "client" part for a synchronous client.
 * Will be called each time a message is transmitted.
 */
static void send_data(char *addr_str, char *data,
                        unsigned int delay)
{

    sock_udp_ep_t local = SOCK_IPV6_EP_ANY;
    sock_udp_ep_t remote = SOCK_IPV6_EP_ANY;
    sock_udp_t udp_socket;

    ssize_t len;

    /* NOTE FIXME: Only a buffer for receiving and sending data? */

    ssize_t pckt_rcvd_size = DTLS_MAX_BUF;
    uint8_t pckt_rcvd[DTLS_MAX_BUF];
    ssize_t app_data_buf = 0; /* The size of the Upper layer packet to send */

    DEBUG("(client) DBG: Synchronous mode enabled\n");

    local.port = (unsigned short) CLIENT_PORT;
    remote.port = (unsigned short) SERVER_DEFAULT_PORT;

    /* Parse <IPv6-address>[%netif] */
    int iface = ipv6_addr_split_iface(addr_str);
    if (iface == -1) {
        if (gnrc_netif_numof() == 1) {
            /* assign the single interface found in gnrc_netif_numof() */
            remote.netif = (uint16_t)gnrc_netif_iter(NULL)->pid;
        }
        else {
            remote.netif = SOCK_ADDR_ANY_NETIF;
        }
    }
    else {
        if (gnrc_netif_get_by_pid(iface) == NULL) {
            puts("(client) ERROR: interface not valid");
            return ;
        }
        remote.netif = iface;
    }

    if (ipv6_addr_from_str( (ipv6_addr_t *)remote.addr.ipv6, addr_str) == NULL) {
        puts("(client) Error: unable to parse destination address");
        return;
    }

    if (sock_udp_create(&udp_socket, &local, NULL, 0) != 0) {
        puts("(client) Error: Unable to create UDP sock");
        return;
    }

    app_data_buf = strlen(data);
    DEBUG("(client) DBG: app_data_buf %i\n", app_data_buf);

    len = sock_udp_send(&udp_socket, data, app_data_buf, &remote);
    if (!(len > 0)) {
        printf("Error: Unable to send message (code %i)", len);
    }

    /* Awaits for any type of ACK */
    len = sock_udp_recv(&udp_socket, pckt_rcvd, pckt_rcvd_size,
                                delay, &remote);

    if (len >=  0){
        printf("(client) got ACK\n");
    }
#ifdef ENABLE_DEBUG
    else if (len == -ETIMEDOUT){
        puts("(client) Not answer from server (timeout)");
    }
    else{
        printf("(server) ERROR: unexpected code error: %i", pckt_rcvd_size);
    }
#endif /* ENABLE_DEBUG */

    sock_udp_close(&udp_socket);

    return;
}

#endif /* SOCK_HAS_ASYNC */

int client_cmd(int argc, char **argv)
{
#ifdef SOCK_HAS_ASYNC
  /* Awaiting time (uS) for receiving ACK from the server */
  /* WARNING: Too low makes the client thread not sleepign enough*/
  uint32_t delay = SERVER_ANSWER_TIMEOUT * 2;
#else /* SOCK_HAS_ASYNC */
  /* Hardcore delay between the sending of each UDP record */
  uint32_t delay = 1000;
#endif

    if (argc < 3) {
        printf("usage: %s <addr>[%%netif] <data> [<delay in us>]\n", argv[0]);
        return 1;
    }
    else if (argc > 3) {
        delay = (uint32_t)atoi(argv[3]);
    }

#ifdef SOCK_HAS_ASYNC
    _start_asyn_thread(argv[1], argv[2], delay);
#else
    send_data(argv[1], argv[2], delay);
#endif
    return 0;
}
