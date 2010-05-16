#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dl_file.h"

extern FILE *log_file;
#include <errno.h>
#include "util/logger.h"
struct dl_file*
dl_file_new (const char *path, const int size) {
    struct dl_file *file;

    file = malloc (sizeof (struct dl_file));
    if (!file)
        return NULL;

    file->next = NULL;
    file->path = strdup (path);

#if 0
    if (stat (path, &buf) < 0) {
        log_failure (log_file, "STAT FAILED : %s", strerror (errno));
        return NULL;
    }
    file->total_size = buf.st_size;
#endif
    file->total_size = size;

    return file;
}

struct dl_file *dl_file_add (struct dl_file *q, struct dl_file *f) {
    if (q)
        f->next = q;

    return f;
}

struct dl_file*
dl_file_remove (struct dl_file *q, struct dl_file *f) {
    struct dl_file *tmp, *t;

    if (!q)
        return NULL;

    if (!f)
        return q;

    for (tmp = q, t = NULL; 
         tmp; 
         t = tmp, tmp = tmp->next) {
        if (tmp == f) {
            /* First element */
            if (!t) {
                return tmp->next;
            }
            /* Not the first element */
            t->next = tmp->next;
            return q;
        }
    }
    
    /* File not in the list */
    return q;
}

void
dl_file_free (struct dl_file *f) {
    if (!f)
        return;
    
    if (f->path) {
        free (f->path);
        f->path = NULL;
    }

    f->next = NULL;
    free (f);
}
