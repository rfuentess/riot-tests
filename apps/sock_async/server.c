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
 * @brief       UDP sock asynchronous server side
 *
 * @author      Raul Fuentes <raul.fuentes-samaniego@inria.fr>
 *
 * @}
 */


#include <stdio.h>
#include <inttypes.h>

#include "timex.h"
#include "xtimer.h"
#include "msg.h"

#include "net/sock/udp.h"

#ifdef SOCK_HAS_ASYNC
#include "event/timeout.h"
#endif

#define ENABLE_DEBUG (1)
#include "debug.h"

#ifndef SERVER_DEFAULT_PORT
#define SERVER_DEFAULT_PORT 20220
#endif

#define _STOP_SERVER_IPHC_MSG 0x4001 /* Custom IPC type msg. */

static kernel_pid_t _server_pid = KERNEL_PID_UNDEF;

/* FIXME Identify if 8 is enough */
#define SERVER_QUEUE_SIZE (8U)
char _server_stack[THREAD_STACKSIZE_MAIN + THREAD_EXTRA_STACKSIZE_PRINTF];

/* WARNING FIXME Remove this line (once DTLS is integrated)*/
#ifndef DTLS_MAX_BUF
#define DTLS_MAX_BUF 140
#endif


#ifdef SOCK_HAS_ASYNC

/* NOTE: Require discussion */

static void _asyn_handler(event_t *arg);

static event_queue_t udp_event_queue;
static event_t _asyn_event = { .handler = _asyn_handler };

/* NOTE: Only declared as global for SOCK_HAS_ASYNC */
static uint8_t pckt_rcvd[DTLS_MAX_BUF];
static sock_udp_t udp_socket;

static void _asyn_handler(event_t *arg)
{
    /* NOTE: Not fully tested  */
    sock_event_t *event = (sock_event_t *)arg;

    DEBUG("(server) DBG: Event received! (type 0x%08x) \n", event->type);

    /* TODO: Generate our own TLS_APP_DATA_RECV */
    if (event->type & SOCK_EVENT_RECV) {
        sock_udp_ep_t remote;
        ssize_t res;

        while ((res = sock_udp_recv(event->sock, pckt_rcvd, sizeof(pckt_rcvd),
                                    0, &remote)) >= 0) {
            printf("(server) Data received -- ");
            ssize_t i;
            for (i = 0; i < DTLS_MAX_BUF; i++) {
                printf("%c", pckt_rcvd[i]);
            }
            printf(" ---\n");

            /* Send back an ACK */
            sock_udp_send(event->sock, "ACK", sizeof("ACK"), &remote);
        }
    }
}

#endif /* SOCK_HAS_ASYNC */

void *_server_wrapper(void *arg)
{
    (void) arg;

    /* Variables for IPC */
    bool active = true;
    msg_t _reader_queue[SERVER_QUEUE_SIZE];
    msg_t msg;

#ifndef SOCK_HAS_ASYNC
    uint8_t pckt_rcvd[DTLS_MAX_BUF];
    sock_udp_t udp_socket;
    sock_udp_ep_t remote = SOCK_IPV6_EP_ANY;
    ssize_t pckt_rcvd_size = DTLS_MAX_BUF;
#endif

    /* FIXME */

    sock_udp_ep_t local = SOCK_IPV6_EP_ANY;

    /* Prepare (thread) messages reception */
    msg_init_queue(_reader_queue, SERVER_QUEUE_SIZE);

    local.port = SERVER_DEFAULT_PORT;

    ssize_t res = sock_udp_create(&udp_socket, &local, NULL, 0);
    if (res == -1) {
        puts("ERROR: Unable create sock.");
        return (void *) NULL;
    }

    /* FIXME: This ifdef should be different but until PR 8149 is not ready
     *        I'm not mixing the code.
     *
     *        Alternatively, an #ifdef for two different _server_wrapper
     *        should be considered if _STOP_SERVER_IPHC_MSG is used as an
     *        event type.
     */
#ifndef SOCK_HAS_ASYNC

    DEBUG("(server) DBG: Synchronous mode enabled\n");

    while (active) {

        msg_try_receive(&msg); /* Check if we got an (thread) message */
        if (msg.type == _STOP_SERVER_IPHC_MSG) {
            active = false;
        }
        else { /* Normal operation */

            /* NOTE: The 10 seconds time is for reaching the IPC msgs again. */
            /* FIXME */
            pckt_rcvd_size = sock_udp_recv(&udp_socket, pckt_rcvd,
                                           sizeof(pckt_rcvd),
                                           10 * US_PER_SEC, &remote);

            if (pckt_rcvd_size >= 0) {

                printf("(server) Data received -- ");
                ssize_t i;
                for (i = 0; i < pckt_rcvd_size; i++) {
                    printf("%c", pckt_rcvd[i]);
                }
                printf(" ---\n");

                /* Send back an ACK */
                sock_udp_send(&udp_socket, "ACK", sizeof("ACK"), &remote);
            }
#ifdef ENABLE_DEBUG
            else {
                switch (pckt_rcvd_size) {
                    case -ENOBUFS:
                        puts("(server) ERROR: Buffer space not enough large!");
                        break;

                    case -EADDRNOTAVAIL:
                        puts("(server) ERROR: Local Socket NULL");
                        break;

                    /* NOTE: Actually, those are OK for this test */
                    case -EAGAIN:
                    case -ETIMEDOUT:
                        break;

                    case -ENOMEM:
                        puts("(server) ERROR: Memory overflow! (pckt_rcvd)");
                        break;

                    case -EINVAL:
                        puts("(server) ERROR: remote sock is not properly initialized");
                        break;

                    case -EPROTO:
                        puts("(server) ERROR: source address of received packet did not equal the remote");
                        break;

                    default:
                        printf("(server) ERROR: unexpected code error: %i", pckt_rcvd_size);
                        break;
                }   /* Switch */
            }       /* else (sock_udp_recv) */
#endif /* ENABLE_DEBUG */
        } /* else (msg_try_receive) */
    } /* While */

#else /* SOCK_HAS_ASYNC */

    event_queue_init(&udp_event_queue);
    sock_udp_set_event_queue(&udp_socket, &udp_event_queue,
                             _asyn_event.handler);

    (void) _server_pid;
    event_t *event;

    /* NOTE: There are two alternatives:
     * 1) Using IPHC messages and event_get() (Default right now)
     * 2) Convert the _STOP_SERVER_IPHC_MSG into a event type and use
     *    event_loop().
     *
     * Both of them implies a while(1) running in this thread.
     */

    DEBUG("(server) DBG: Asynchronous mode enabled\n");

    while (active) {
        msg_try_receive(&msg); /* Check if we got an (thread) message */
        if (msg.type == _STOP_SERVER_IPHC_MSG) {
            active = false;
            sock_udp_set_event_queue(&udp_socket, NULL, NULL);
            DEBUG("(server) DBG: sock udp events disabled\n");
        }

        /* WARNING: Not fullly tested */
        /* NOTE: The server is now in asynchronous listening mode(?) */
        event = event_get(&udp_event_queue);
        if (event){
            event->handler(event);
        }

        xtimer_usleep(1000);  /* FIXME: Magic number for now */
    }
#endif

    sock_udp_close(&udp_socket);
    msg_reply(&msg, &msg); /* Basic answer to the main thread */
    return (void *) NULL;
}

static void _start_server(void)
{
    /* Only one instance of the server */
    if (_server_pid != KERNEL_PID_UNDEF) {
        puts("(server) Error: server already running");
        return;
    }

    /* The server is initialized */
    _server_pid = thread_create(_server_stack, sizeof(_server_stack),
                                     THREAD_PRIORITY_MAIN - 1,
                                     THREAD_CREATE_STACKTEST,
                                     _server_wrapper, NULL,
                                     "sock_server");

    /* Uncommon but better be sure */
    if (_server_pid == EINVAL) {
        puts("(server) ERROR: Thread invalid");
        _server_pid = KERNEL_PID_UNDEF;
        return;
    }

    if (_server_pid == EOVERFLOW) {
        puts("(server) ERROR: Thread overflow!");
        _server_pid = KERNEL_PID_UNDEF;
        return;
    }

    return;
}

static void _stop_server(void)
{
    /* check if server is running at all */
    if (_server_pid == KERNEL_PID_UNDEF) {
        puts("Error: DTLS server is not running");
        return;
    }

    /* prepare the stop message */
    msg_t m = {
        thread_getpid(),
        _STOP_SERVER_IPHC_MSG,
        { NULL }
    };

    DEBUG("(server) DBG: Stopping server...\n");
    /* send the stop message to thread AND wait for (any) answer */
    msg_send_receive(&m, &m, _server_pid);

    _server_pid = KERNEL_PID_UNDEF;

    puts("Success: UDP server stopped");
}

int server_cmd(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s start|stop\n", argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "start") == 0) {
        _start_server();
    }
    else if (strcmp(argv[1], "stop") == 0) {
        _stop_server();
    }
    else {
        puts("Error: invalid command");
    }
    return 0;
}
