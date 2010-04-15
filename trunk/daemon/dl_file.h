#ifndef DL_FILE_H
#define DL_FILE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct dl_file {
    struct dl_file *next;
    /* Complete path (path + name) */
    char           *path;
    off_t          total_size;
};

/*
 * Creates a new dl_file structure. 
 * Path is the complete path of the file (including its name).
 */
struct dl_file *dl_file_new (const char *path, const int size);

/*
 * "q" is a list of struct dl_file structures, to which "f" will be added.
 * Returns the new queue.
 */
struct dl_file *dl_file_add (struct dl_file *q, struct dl_file *f);

/*
 *
 */
struct dl_file *dl_file_remove (struct dl_file *q, struct dl_file *f);
void dl_file_free (struct dl_file *f);
#endif /* DL_FILE_H */
