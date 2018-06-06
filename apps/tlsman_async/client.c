/*
 * Copyright (C) 2018 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       tlsman test application (client side)
 *
 * Small test for TLSMAN. Many definitions defined here are also available at
 * sock_secure (and are intended to be used in standard applications)
 *
 * @author      Raul Fuentes <raul.fuentes-samaniego@inria.fr>
 *
 * @}
 */


#include <stdio.h>
#include <assert.h>

#include "net/sock/udp.h"
#include "net/tlsman.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#define READER_CLIENT_QUEUE_SIZE (8U)

/* Expected IPC messages  */
#define IPHC_MSG_STOP           0x4000
#define IPHC_MSG_START_LISTEN   0x4001
#define IPHC_MSG_STOP_LISTEN    0x4002
#define IPHC_MSG_CONNNECT       0x4003
#define IPHC_MSG_DISCONNECT     0x4004
#define IPHC_MSG_QUIT           0x4005

char client_thread_stack[THREAD_STACKSIZE_MAIN +
                                THREAD_EXTRA_STACKSIZE_PRINTF];

void * client_wrapper(void *arg);


void * client_wrapper(void *arg)
{
  uint8_t *tlsman_flags = (uint8_t *) arg;

  bool active = true;
  msg_t _reader_queue[READER_CLIENT_QUEUE_SIZE];
  msg_t msg;

  puts("Client is running!");
  (void) tlsman_flags;

  /* Prepare (thread) messages reception */
  msg_init_queue(_reader_queue, READER_CLIENT_QUEUE_SIZE);

  while (active) {
    msg_receive(&msg);
    switch(msg.type) {
      case IPHC_MSG_STOP:
        DEBUG("%s: IPHC_MSG_STOP\n",__func__);
      break;

      case IPHC_MSG_START_LISTEN:
        DEBUG("%s: IPHC_MSG_START_LISTEN\n",__func__);

      break;

      case IPHC_MSG_STOP_LISTEN:
        DEBUG("%s: IPHC_MSG_STOP_LISTEN\n",__func__);

      break;

      case IPHC_MSG_CONNNECT:
        DEBUG("%s: IPHC_MSG_CONNNECT\n",__func__);

      break;

      case IPHC_MSG_DISCONNECT:
        DEBUG("%s: IPHC_MSG_DISCONNECT\n",__func__);

      break;

      case IPHC_MSG_QUIT:
        DEBUG("%s: IPHC_MSG_QUIT\n",__func__);

      break;

      default:
        /* FIXME Remove this! (or don't use msg_try_receive()?) */
        DEBUG("unknown IPHC Message! %i\n", msg.type);
    } /* switch */

    msg_reply(&msg, &msg);
  } /* while */

  puts("Client is over!");
  return (void *) NULL;
}
