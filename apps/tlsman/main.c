 #include <stdio.h>

#include "shell.h"
 #include "msg.h"
 #include "net/tlsman.h"

#ifndef DTLS_DEFAULT_PORT
#define DTLS_DEFAULT_PORT 20220 /* DTLS default port */
#endif

/* List of acceptable cipher suites (T) */
/* NOTE: For now, only CoAP Secure candidates (RFC 7252 9.1.3) */
#define SECURE_CIPHER_PSK_IDS (0xC0A8)
#define SECURE_CIPHER_RPK_IDS (0xC0AE)
#define SECURE_CIPHER_LIST { SECURE_CIPHER_PSK_IDS, SECURE_CIPHER_RPK_IDS }

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

extern int client_cmd(int argc, char **argv);
int server_cmd(int argc, char **argv);

static const shell_command_t shell_commands[] = {
    { "client", "Start the testing clientt", client_cmd},
    { "server", "Start the testing server", server_cmd},
    { NULL, NULL, NULL }
};

int main(void)
{

    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("TLSMAN Module example implementation");

    /* The Cipher(s) the application must use (Hardcoded) */
    int chipers[] = SECURE_CIPHER_LIST;
    ssize_t res = tlsman_load_stack(chipers, sizeof(chipers), TLSMAN_FLAG_STACK_DTLS);

    switch (res) {
        case 0:
            /* start shell */
            puts("All up, running the shell now");
            puts("WARNING: Server and Client are one call only!"); /* TODO */
            char line_buf[SHELL_DEFAULT_BUFSIZE];
            shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
        break;

        case TLSMAN_ERROR_TRANSPORT_NOT_SUPPORTED:
            puts("tlsman_load_stack: TLSMAN_ERROR_TRANSPORT_NOT_SUPPORTED");
        case TLSMAN_ERROR_CIPHER_NOT_SUPPORTED:
            puts("tlsman_load_stack: TLSMAN_ERROR_CIPHER_NOT_SUPPORTED");
        default:
            puts("ERROR: Unable to load stack ");
            return -1;
    }

    return 0;
}
