#include <stdlib.h>
#include <pthread.h>
#include "company.h"

typedef struct Node {
    Actor *actor;

    int ref_count;
    pthread_mutex_t ref_mutex;

    struct Node *parent;
    struct Node *next_sibling;
    struct Node *first_child;
    pthread_rwlock_t family_rw_lock;

    int in_use;
} Node;

struct Company {
    Node *list;
    int list_size;
    int nodes_in_use;
    pthread_rwlock_t list_rw_lock;
};

void
node_init (Node *node)
{
    node->actor = NULL;
    node->parent = NULL;
    node->next_sibling = NULL;
    node->first_child = NULL;
    node->in_use = 0;
    pthread_mutex_init(&node->ref_mutex, NULL);
    pthread_rwlock_init(&node->family_rw_lock, NULL);
}

Company *
company_create (int buffer_length)
{
    int i;
    Company *company = malloc(sizeof(*company));

    if (company == NULL)
        goto exit;

    company->list = malloc(sizeof(Node) * buffer_length);

    if (company->list == NULL) {
        free(company);
        company = NULL;
        goto exit;
    }

    company->list_size = buffer_length;
    company->nodes_in_use = 0;
    pthread_rwlock_init(&company->list_rw_lock, NULL);

    for (i = 0; i < company->list_size; i++)
        node_init(&company->list[i]);

exit:
    return company;
}

/*
 * Create an Actor using the given Lua stack. Expects a definition table on top
 * of the stack. Find the first Node not in use and put the Actor pointer (that
 * was created) in it. If the parent_id is >0, add the Actor as the child of
 * that parent.
 *
 * Find the first node not in use and create the Actor in it.
 * Returns -1 if there was an error.
 */
int
company_add_actor (Company *company, lua_State *L, int parent_id)
{
    int i, id = -1;

    pthread_rwlock_wrlock(&company->list_rw_lock);
    if (company->nodes_in_use == company->list_size) {
        /* realloc list */
    }

    for (i = 0; i < company->list_size; i++) {
        if (!company->list[i].in_use) {
            company->list[i].in_use = 1;
            company->nodes_in_use++;
            id = i;
        }
    }
    pthread_rwlock_unlock(&company->list_rw_lock);

    if (id == -1)
        goto exit;

exit:
    return id;
}

void
company_close (Company *company)
{
    int i;

    for (i = 0; i < company->list_size; i++) {
        /* free actor */
    }

    free(company->list);
    free(company);
}
