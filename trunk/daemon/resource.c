#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../util/logger.h"
#include "resource.h"

extern FILE *log_file;

struct resource_tree *resources;
sem_t                resources_lock;

#define MIN(a,b)    (((a)>(b))?(b):(a))

static struct resource *
resource_new (const char *key, const char *ip_port) {
    struct resource     *resource;

    resource = (struct resource *)malloc (sizeof (struct resource));
    if (strlen (key) != RESOURCE_KEY_SIZE) {
        log_failure (log_file, "resource_new (): Wrong key size");
        free (resource);
        return NULL;
    }
    strncpy (resource->key, key, RESOURCE_KEY_SIZE + 1);

    resource->seeders = (struct seeder *)malloc (sizeof (struct seeder));
    strncpy (resource->seeders->ip_port, ip_port, RESOURCE_IP_PORT_SIZE + 1);

    resource->nb_seeders = 1;

    if (sem_init (&resource->seeders_lock, 0, 1) < 0) {
        log_failure (log_file, "resource_new (): Unable to sem_init");
        free (resource->seeders);
        free (resource);
        return NULL;
    }

    return resource;
}

/*
struct resource *
resource_new (const char *filename, const char *key,
                res_size_t size, const char *ip_port) {
    struct resource *resource;
    int filename_size;

    resource = (struct resource *)malloc (sizeof (struct resource));

    if (!resource) {
        printf ("Error allocating resource\n");
        exit (EXIT_FAILURE);
    }

    filename_size = MIN (RESOURCE_FILENAME_SIZE, strlen (filename)) + 1;
    strncpy (resource->filename, filename, filename_size);
    resource->filename[filename_size - 1] = '\0';

    strncpy (resource->key, key, RESOURCE_KEY_SIZE + 1);
    resource->key[RESOURCE_KEY_SIZE] = '\0';

    resource->size = size;

    resource->seeders = (struct seeder *)malloc (sizeof (struct seeder));
    strncpy (resource->seeders->ip_port, ip_port, RESOURCE_IP_PORT_SIZE + 1);

    resource->nb_seeders = 1;

    sem_init (&resource->seeders_lock, 0, 1);

    return resource;
}
*/

static void
resource_free (struct resource *resource) {
    free (resource->seeders);
    sem_destroy (&resource->seeders_lock);
    free (resource);
}

struct resource_tree *
resource_tree_add (struct resource_tree *tree, const char *key,
                    const char *ip_port) {
    int                 bias = 0;
    struct resource     *r;

    if (strlen (ip_port) > RESOURCE_IP_PORT_SIZE) {
        log_failure (log_file, "resource_tree_add (): Wrong ip_port size");
        return tree;
    }

    // If the tree is empty, we create the tree and the resource
    if (tree == NULL) {
        tree = (struct resource_tree *)malloc (sizeof (struct resource_tree));
        if (!tree) {
            log_failure (log_file, "resource_tree_add (): Unable to malloc");
            return NULL;
        }
        tree->this = resource_new (key, ip_port);
        if (!tree->this) {
            free (tree);
            log_failure (log_file,
                        "resource_tree_add (): Unable to resource_new ()");
            return NULL;
        }
        tree->left = NULL;
        tree->right = NULL;
    }
    // If the tree isn't empty, we must recursively search the resource
    else {
        bias = strcmp (key, tree->this->key);
        if (bias < 0) {
            tree->left = resource_tree_add (tree->left, key, ip_port);
        }
        else if (bias > 0) {
            tree->right = resource_tree_add (tree->right, key, ip_port);
        }
        else {
            r = tree->this;
            for (int i = 0; i < r->nb_seeders; i++) {
                // If the seeder is already registered for this resource
                if (strcmp (r->seeders[i].ip_port, ip_port) == 0)
                    return 0; // Nothing to do
            }
            // If the seeder isn't registered, we must add him
            r->seeders = (struct seeder *)realloc (r->seeders,
                                    (r->nb_seeders + 1)*sizeof (struct seeder));
            strcpy (r->seeders[r->nb_seeders].ip_port, ip_port);
            r->nb_seeders++;
        }
    }
    return tree;
}

struct resource_tree *
resource_tree_free (struct resource_tree *tree) {
    if (tree != NULL) {
        if (tree->left != NULL) {
            resource_tree_free (tree->left);
            tree->left = NULL;
        }
        if (tree->right != NULL) {
            resource_tree_free (tree->right);
            tree->right = NULL;
        }
        resource_free (tree->this);
        tree->this = NULL;
        free (tree);
    }
    return NULL;
}

/*
void
resource_add_seeder (struct resource *resource, const char *ip_port) {
    sem_wait (&resource->seeders_lock);
    for (int i = 0; i < resource->nb_seeders; i++)
        if (strncmp (ip_port,
                    resource->seeders[i].ip_port,
                    RESOURCE_IP_PORT_SIZE +1))
            return;
    resource->seeders = (struct seeder *)realloc (resource->seeders,
                        (resource->nb_seeders + 1) * sizeof (struct seeder));
    strncpy (resource->seeders[resource->nb_seeders].ip_port,
                ip_port,
                RESOURCE_IP_PORT_SIZE + 1);
    resource->nb_seeders++;
    sem_post (&resource->seeders_lock);
}

struct resource_tree *
resource_tree_add (struct resource_tree *tree, struct resource *resource) {
    int bias = 0;
    if (tree == NULL) {
        tree = (struct resource_tree *)malloc (sizeof (struct resource_tree));
        tree->this = resource;
        tree->left = NULL;
        tree->right = NULL;
    }
    else {
        bias = strcmp (resource->key, tree->this->key);
        if (bias < 0) {
            tree->left = resource_tree_add (tree->left, resource);
        }
        else if (bias > 0) {
            tree->right = resource_tree_add (tree->right, resource);
        }
        else {
            printf ("Error : key already used\n");
        }
    }
    return tree;
}

struct resource *
resource_tree_get_by_key (struct resource_tree *tree, const char *key) {
    int compare;

    if (!tree)
        return NULL;

    compare = strncmp (tree->this->key, key, RESOURCE_KEY_SIZE + 1);
    if (!compare)
        return tree->this;
    else if (compare < 0)
        return resource_tree_get_by_key (tree->left, key);
    else
        return resource_tree_get_by_key (tree->right, key);
}

void
resource_tree_to_client (struct resource_tree *tree, struct client *c) {
    char line[RESOURCE_KEY_SIZE + RESOURCE_FILENAME_SIZE + 27];

    if (tree) {
        sprintf (line,
                " < %s %s %20llu\n",
                tree->this->key,
                tree->this->filename,
                tree->this->size);
        resource_tree_to_client (tree->left, c);
        client_send (c, line);
        resource_tree_to_client (tree->right, c);
    }
}

void
resource_tree_free (struct resource_tree *tree) {
    if (tree != NULL) {
        if (tree->left != NULL) {
            resource_tree_free (tree->left);
            tree->left = NULL;
        }
        if (tree->right != NULL) {
            resource_tree_free (tree->right);
            tree->right = NULL;
        }
        resource_free (tree->this);
        tree->this = NULL;
        free (tree);
    }
}
*/
