#include <stdlib.h>
#include <stdio.h>

void
do_help (char *msg) {
    free (msg);
    printf("\
-= HELP =-\n\
set                         lists the options available\n\
set [OPTION=VALUE] [...]    changes the value of an option\n\
help                        displays this help\n\
list                        lists the resources available on the network\n\
get KEY                     gets the resource identified by KEY\n\
info                        displays statistics\n\
download                    information about current downloads\n\
upload                      information about current uploads\n\
connect IP:PORT             connects to another program\n\
raw IP:PORT CMD             sends a command to another program\n\
quit                        exits the program\n\
");
}

