#include <stdlib.h>             // malloc ()
#include <string.h>             // strdup ()

#include "callback_argument.h"  // struct callback_argument

struct callback_argument*
cba_new (char *cmd, int client_socket) {
    struct callback_argument *cba;
    cba = malloc (sizeof (struct callback_argument));
    if (!cba)
        return NULL;
    cba->cmd           = strdup (cmd);
    cba->client_socket = client_socket;
    return cba;
}

void
cba_free (struct callback_argument *cba) {
    if (!cba)
        return;

    if (cba->cmd) {
        free (cba->cmd);
        cba->cmd = NULL;
    }

    free (cba);
    cba = NULL;
}
