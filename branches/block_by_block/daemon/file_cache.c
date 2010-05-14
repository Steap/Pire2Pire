#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../util/logger.h"
#include "file_cache.h"

extern FILE *log_file;

#define MIN(a,b)    (((a)>(b))?(b):(a))

/*
 * Creates a struct file_cache.
 */
static struct file_cache *
file_cache_new (const char *filename,
                const char *key,
                file_size_t size,
                const char *ip,
                int port) {
    struct file_cache *file_cache;

    file_cache = (struct file_cache *)malloc (sizeof (struct file_cache));

    if (strlen (filename) > FILE_NAME_SIZE) {
        log_failure (log_file, "file_cache_new (): Too long filename");
        free (file_cache);
        return NULL;
    }
    strncpy (file_cache->filename, filename, FILE_NAME_SIZE + 1);

    if (strlen (key) != FILE_KEY_SIZE) {
        log_failure (log_file, "file_cache_new (): Wrong key size");
        free (file_cache);
        return NULL;
    }
    strncpy (file_cache->key, key, FILE_KEY_SIZE + 1);

    file_cache->size = size;

    file_cache->seeders = (struct seeder *)malloc (sizeof (struct seeder));
    strncpy (file_cache->seeders->ip, ip, FILE_IP_SIZE + 1);
    file_cache->seeders->port = port;
    file_cache->seeders->next = NULL;
    
	if (sem_init (&file_cache->seeders_lock, 0, 1) < 0) {
        log_failure (log_file, "file_cache_new (): Unable to sem_init");
        free (file_cache->seeders);
        free (file_cache);
        return NULL;
    }
    
	file_cache->hole_map = hole_map_new (0, size - 1);
	
	if (sem_init (&file_cache->hole_map_lock, 0, 1) < 0) {
        log_failure (log_file, "file_cache_new (): Unable to sem_init");
        free (file_cache->seeders);
        hole_map_free(file_cache->hole_map);
        free (file_cache);
        return NULL;
    }
    

    file_cache->left = NULL;
    file_cache->right = NULL;

    return file_cache;
}

/*
 * Frees properly a struct file_cache. Please note it does not look at its left
 * and right children.
 */
static void
__file_cache_free (struct file_cache *file_cache) {
    struct seeder   *to_be_freed;
    struct seeder   *next_to_be_freed;

    if (!file_cache)
        return;

    sem_wait (&file_cache->seeders_lock);
    to_be_freed = file_cache->seeders;
    while (to_be_freed) {
        next_to_be_freed = to_be_freed->next;
        free (to_be_freed);
        to_be_freed = next_to_be_freed;
    }
    sem_destroy (&file_cache->seeders_lock);
    
    sem_wait (&file_cache->hole_map_lock);
    hole_map_free (file_cache->hole_map);
    sem_destroy (&file_cache->hole_map_lock);

    free (file_cache);
}

struct file_cache *
file_cache_add (struct file_cache *tree,
                const char *filename,
                const char *key,
                file_size_t size,
                const char *ip,
                int port) {
    /* bias determines if we go left or right in the binary tree */
    int                 bias = 0;
    struct seeder       *s;

    // If the tree is empty, we create it
    if (!tree) {
        tree = file_cache_new (filename, key, size, ip, port);
        if (!tree) {
            log_failure (log_file,
                        "file_cache_add (): Unable to file_cache_new ()");
            return NULL;
        }
    }
    // If the tree isn't empty, recursive call to find the right place
    else {
        bias = strcmp (key, tree->key);
        if (bias < 0) {
            tree->left = file_cache_add (tree->left,
                                            filename,
                                            key,
                                            size,
                                            ip,
                                            port);
        }
        else if (bias > 0) {
            tree->right = file_cache_add (tree->right,
                                            filename,
                                            key,
                                            size,
                                            ip, port);
        }
        /* If a node with the same key already exists */
        else {
            /* Search if the seeder is already registered, if he is, leave */
            sem_wait (&tree->seeders_lock);
            for (s = tree->seeders; s; s = s->next)
                if (strcmp (s->ip, ip) == 0 && s->port == port)
                    return tree;
            sem_post (&tree->seeders_lock);

            /* If the seeder isn't registered, we must add him */
            s = (struct seeder *)malloc (sizeof (struct seeder));
            if (!s) {
                log_failure (log_file,
                            "file_cache_add (): Unable to allocate seeder");
                return tree;
            }
            strcpy (s->ip, ip);
            s->port = port;
            sem_wait (&tree->seeders_lock);
            s->next = tree->seeders;
            tree->seeders = s;
            sem_post (&tree->seeders_lock);
        }
    }
    return tree;
}

/* FIXME: Invalid free... */
void
file_cache_free (struct file_cache *tree) {
    if (tree) {
        file_cache_free (tree->left);
        file_cache_free (tree->right);
        __file_cache_free (tree);
    }
}

/*
void
file_cache_add_seeder (struct file_cache *file_cache, const char *ip_port) {
    sem_wait (&file_cache->seeders_lock);
    for (int i = 0; i < file_cache->nb_seeders; i++)
        if (strncmp (ip_port,
                    file_cache->seeders[i].ip_port,
                    FILE_IP_PORT_SIZE +1))
            return;
    file_cache->seeders = (struct seeder *)realloc (file_cache->seeders,
                        (file_cache->nb_seeders + 1) * sizeof (struct seeder));
    strncpy (file_cache->seeders[file_cache->nb_seeders].ip_port,
                ip_port,
                FILE_IP_PORT_SIZE + 1);
    file_cache->nb_seeders++;
    sem_post (&file_cache->seeders_lock);
}

struct file_cache_tree *
file_cache_tree_add (struct file_cache_tree *tree, struct file_cache *file_cache) {
    int bias = 0;
    if (tree == NULL) {
        tree = (struct file_cache_tree *)malloc (sizeof (struct file_cache_tree));
        tree->this = file_cache;
        tree->left = NULL;
        tree->right = NULL;
    }
    else {
        bias = strcmp (file_cache->key, tree->this->key);
        if (bias < 0) {
            tree->left = file_cache_tree_add (tree->left, file_cache);
        }
        else if (bias > 0) {
            tree->right = file_cache_tree_add (tree->right, file_cache);
        }
        else {
            printf ("Error : key already used\n");
        }
    }
    return tree;
}
*/

struct file_cache *
file_cache_get_by_key (struct file_cache *tree, const char *key) {
    int compare;

    if (!tree)
        return NULL;

    compare = strcmp (key, tree->key);
    if (!compare)
        return tree;
    else if (compare < 0)
        return file_cache_get_by_key (tree->left, key);
    else
        return file_cache_get_by_key (tree->right, key);
}

/*
void
file_cache_tree_to_client (struct file_cache_tree *tree, struct client *c) {
    char line[FILE_KEY_SIZE + FILE_FILENAME_SIZE + 27];

    if (tree) {
        sprintf (line,
                " < %s %s %20llu\n",
                tree->this->key,
                tree->this->filename,
                tree->this->size);
        file_cache_tree_to_client (tree->left, c);
        client_send (c, line);
        file_cache_tree_to_client (tree->right, c);
    }
}

void
file_cache_tree_free (struct file_cache_tree *tree) {
    if (tree != NULL) {
        if (tree->left != NULL) {
            file_cache_tree_free (tree->left);
            tree->left = NULL;
        }
        if (tree->right != NULL) {
            file_cache_tree_free (tree->right);
            tree->right = NULL;
        }
        file_cache_free (tree->this);
        tree->this = NULL;
        free (tree);
    }
}
*/
