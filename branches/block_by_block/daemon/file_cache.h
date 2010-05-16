#ifndef FILE_H
#define FILE_H

#include <netinet/in.h>

#include <semaphore.h>

#include "client.h"
#include "daemon.h"

#include "hole_map.h"

#define FILE_NAME_SIZE      256
#define FILE_KEY_SIZE       32
#define FILE_IP_SIZE        INET_ADDRSTRLEN

typedef long unsigned int file_size_t;

struct seeder {
    char            ip[FILE_IP_SIZE + 1];
    int             port;
    
    //struct daemon	*seed;
    struct seeder   *next;
};

struct file_cache {
    char                filename[FILE_NAME_SIZE + 1];
    char                key[FILE_KEY_SIZE + 1];
    file_size_t          size;
    struct seeder       *seeders;
    sem_t               seeders_lock;
    struct hole_map			*hole_map;
    sem_t				hole_map_lock;
    struct file_cache   *left;
    struct file_cache   *right;
};

struct file_cache *file_cache_add (struct file_cache *tree,
                                    const char *filename,
                                    const char *key,
                                    file_size_t size,
                                    const char *ip,
                                    int port);

void file_cache_free (struct file_cache *tree);

/*
void
file_cache_add_seeder (struct file_cache *file_cache, const char *ip_port);

struct file_cache_tree *
file_cache_tree_add (struct file_cache_tree *tree, struct file_cache *file_cache);
*/

struct file_cache *
file_cache_get_by_key (struct file_cache *tree, const char *key);

/*
void
file_cache_tree_to_client (struct file_cache_tree *tree, struct client *c);

void
file_cache_tree_free (struct file_cache_tree *tree);
*/

#endif//FILE_H
