#ifndef RESOURCE_H
#define RESOURCE_H

#include <semaphore.h>

#include "client.h"
#include "daemon.h"

#define RESOURCE_FILENAME_SIZE  255
#define RESOURCE_KEY_SIZE       32
#define RESOURCE_IP_PORT_SIZE   22  //FIXME: IPv6

typedef long long unsigned int res_size_t;

struct seeder {
    char    ip_port[RESOURCE_IP_PORT_SIZE + 1];
};

struct resource {
    char                filename[RESOURCE_FILENAME_SIZE + 1];
    char                key[RESOURCE_KEY_SIZE + 1];
    res_size_t          size;
    struct seeder       *seeders;
    int                 nb_seeders;
    sem_t               seeders_lock;
};

struct resource_tree {
    struct resource         *this;
    struct resource_tree    *left;
    struct resource_tree    *right;
};

struct resource *
resource_new (const char *name, const char *key,
                res_size_t size, const char *ip_port);

void
resource_free (struct resource *resource);

void
resource_add_seeder (struct resource *resource, const char *ip_port);

struct resource_tree *
resource_tree_add (struct resource_tree *tree, struct resource *resource);

struct resource *
resource_tree_get_by_key (struct resource_tree *tree, const char *key);

void
resource_tree_to_client (struct resource_tree *tree, struct client *c);

void
resource_tree_free (struct resource_tree *tree);

#endif//RESOURCE_H
