#ifndef IS_CMD_H
#define IS_CMD_H 1

/*
 * @msg : the msg rceived. There is no '\n' at the end.
 */
#define IS_CMD(msg, cmd)                                               \
        (strncmp (msg, cmd, strlen (cmd)) == 0 &&                      \
            (strlen (msg) == strlen (cmd) || msg[strlen(cmd)] == ' ')) \

#endif /* IS_CMD_H */
    
    
