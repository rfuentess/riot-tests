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
 * @brief       Example application for demonstrating the sock_secure module
 *
 * @author      Raul Fuentes <raul.fuentes-samaniego@inria.fr>
 *
 * @}
 */

 #include <stdio.h>

 #include "shell.h"
 #include "msg.h"

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

extern int client_cmd(int argc, char **argv);
extern int server_cmd(int argc, char **argv);

#define SEND_DESCRIPTION "Send data by means of (plaintext) UDP"
#define SERVER_DESCRIPTION "Start or stop a (plaintext) UDP server"

/* TODO: (D)TLS renegotiation command */
/* TODO: (D)TLS client ending command */

static const shell_command_t shell_commands[] = {
    { "send", SEND_DESCRIPTION, client_cmd },
    { "server", SERVER_DESCRIPTION, server_cmd },
    { NULL, NULL, NULL }
};

int main(void)
{
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("RIOT sock udp asynchronous testing implementation");

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
