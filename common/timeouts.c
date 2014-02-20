#include "timeouts.h"
#include "smp.h"

extern time_t now;

#define TIMEOUTS_LINK_RING_SIZE 512
static timeout_t *timeout_links[TIMEOUTS_LINK_RING_SIZE] = {0};
static timeout_t *timeout_link_ends[TIMEOUTS_LINK_RING_SIZE] = {0};

timeout_t *add_timeout(void *ptr, int timeout, timeout_handle_cb handle)
{
    if(timeout < 1 || !handle) {
        return NULL;
    }

    timeout_t *n = malloc(sizeof(timeout_t));

    if(!n) {
        return NULL;
    }

    n->handle = handle;
    n->ptr = ptr;
    n->timeout = now + timeout;
    n->uper = NULL;
    n->next = NULL;

    int k = n->timeout % TIMEOUTS_LINK_RING_SIZE;

    if(timeout_link_ends[k] == NULL) {
        timeout_links[k] = n;
        timeout_link_ends[k] = n;

    } else { // add to link end
        timeout_link_ends[k]->next = n;
        n->uper = timeout_link_ends[k];
        timeout_link_ends[k] = n;
    }

    return n;
}

int check_timeouts()
{
    int k = now % TIMEOUTS_LINK_RING_SIZE;
    timeout_t *m = NULL, *n = NULL;
    int b = 1;

    while(1) {
        b = 1;
        m = timeout_links[k];
        n = NULL;

        while(m) {
            n = m;
            m = m->next;

            if(now >= n->timeout) {
                n->handle(n->ptr);
                b = 0;
                break;
            }
        }

        if(b) {
            break;
        }
    }

    return 1;
}

void delete_timeout(timeout_t *n)
{
    if(!n) {
        return;
    }

    int k = n->timeout % TIMEOUTS_LINK_RING_SIZE;

    if(n->uper) {
        ((timeout_t *) n->uper)->next = n->next;

    } else {
        timeout_links[k] = n->next;
    }

    if(n->next) {
        ((timeout_t *) n->next)->uper = n->uper;

    } else {
        timeout_link_ends[k] = n->uper;
    }

    free(n);
}

void update_timeout(timeout_t *n, int timeout)
{
    if(!n) {
        return;
    }

    int k = n->timeout % TIMEOUTS_LINK_RING_SIZE;

    if(n->uper) {
        ((timeout_t *) n->uper)->next = n->next;

    } else {
        timeout_links[k] = n->next;
    }

    if(n->next) {
        ((timeout_t *) n->next)->uper = n->uper;

    } else {
        timeout_link_ends[k] = n->uper;
    }

    if(timeout < 1) {
        free(n);
        return;
    }

    n->timeout = now + timeout;
    n->uper = NULL;
    n->next = NULL;

    k = n->timeout % TIMEOUTS_LINK_RING_SIZE;

    if(timeout_link_ends[k] == NULL) {
        timeout_links[k] = n;
        timeout_link_ends[k] = n;

    } else { // add to link end
        timeout_link_ends[k]->next = n;
        n->uper = timeout_link_ends[k];
        timeout_link_ends[k] = n;
    }
}
