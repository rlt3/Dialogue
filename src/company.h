#ifndef DIALOGUE_COMPANY
#define DIALOGUE_COMPANY

#include "actor.h"

/*
 * Create the Company tree with the following options.
 */
int
company_create (int base_actors, int max_actors, int base_children);

int 
company_add (lua_State *L, int parent, int thread_id);

void
company_set (lua_State *L);

int
company_remove (int id, int is_delete);

void *
company_ref (int id);

int
company_deref (int id);

/*
 * Close the Company's tree.
 */
void
company_close ();

#endif
