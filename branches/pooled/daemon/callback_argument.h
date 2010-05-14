#ifndef CALLBACK_ARGUMENT_H
#define CALLBACK_ARGUMENT_H

struct callback_argument {
    char          *cmd;
    int           client_socket;
};


struct callback_argument* cba_new (char *cmd, int client_socket);
void                      cba_free (struct callback_argument *cba);
#endif /* CALLBACK_ARGUMENT_H */
