#include <stdio.h>
#include <stdlib.h>

#include "callback_argument.h"


struct callback_argument*
cba_new (char *cmd, int client_socket) {
    struct callback_argument *cba;
    cba = malloc (sizeof (struct callback_argument));
    if (!cba)
        return NULL;
    cba->cmd           = cmd;
    cba->client_socket = client_socket;
    return cba;
}

void
cba_free (struct callback_argument *cba) {
    if (!cba)
        return;
    if (cba->cmd)
        free (cba->cmd);
    free (cba);
}
