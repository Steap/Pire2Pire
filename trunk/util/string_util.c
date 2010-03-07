#include <string.h>

void
string_remove_trailer (char *msg) {
    char *to_be_replaced;

    to_be_replaced = strchr(msg, '\r');
    if (to_be_replaced != NULL) {
        *to_be_replaced = '\0';
    }
    to_be_replaced = strchr(msg, '\n');
    if (to_be_replaced != NULL) {
        *to_be_replaced = '\0';
    }
}
