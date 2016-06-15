/*  =========================================================================
    zecho - Implements the echo algorithms

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of CZMQ, the high-level C binding for 0MQ:
    http://czmq.zeromq.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    zecho - Implements the echo algorithms
@discuss
@end
*/

#include "zlog_classes.h"

//  Structure of our class

struct _zecho_t {
    int recv_msg;     //  Counts the number of received mesages.
    char *father;     //  Holds the ID of the first peer that did send an
                      //  echo message
    zyre_t *node;      //  Own zyre handle
};


//  --------------------------------------------------------------------------
//  Create a new zecho

zecho_t *
zecho_new (zyre_t *node)
{
    zecho_t *self = (zecho_t *) zmalloc (sizeof (zecho_t));
    assert (self);
    //  Initialize class properties here
    self->recv_msg = 0;
    self->father = NULL;
    self->node = node;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zecho

void
zecho_destroy (zecho_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zecho_t *self = *self_p;
        //  Free class properties here
        zstr_free (&self->father);
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Initiate the echo algorithm

void
zecho_init (zecho_t *self) {
    self->father = strdup("initiator");

    zlist_t *groups = zyre_own_groups (self->node);
    const char *group = (const char *) zlist_first (groups);
    while (group) {
        printf("Send to group: %s\n", group);
        zlist_t *neighbors = zyre_peers_by_group (self->node, group);
        printf("Neighbors in group: %lu\n", zlist_size (neighbors));
        char *neighbor = (char *) zlist_first (neighbors);
        while (neighbor) {
            if (!streq (neighbor, self->father)) {
                //  Send token to neighbor
                zmsg_t *token = zmsg_new ();
                zmsg_addstr (token, "tok");
                zyre_whisper (self->node, neighbor, &token);
            }
            //  Get next item in list
            neighbor = (char *) zlist_next (neighbors);
        }
        zlist_destroy (&neighbors);

        group = (const char *) zlist_next (groups);
    }
    zlist_destroy (&groups);
}


//  --------------------------------------------------------------------------

static unsigned long
s_zecho_neighbor_count (zecho_t *self)
{
    unsigned long neighbors_count = 0;
    zlist_t *groups = zyre_own_groups (self->node);
    const char *group = (const char *) zlist_first (groups);
    while (group) {
        zlist_t *neighbors = zyre_peers_by_group (self->node, group);
        neighbors_count += zlist_size (neighbors);

        group = (const char *) zlist_next (groups);
        zlist_destroy (&neighbors);
    }
    zlist_destroy (&groups);
    return neighbors_count;
}


//  --------------------------------------------------------------------------
//  Handle a received echo token

void
zecho_recv (zecho_t *self, zyre_event_t *token)
{
    self->recv_msg++;

    if (!self->father) {
        self->father = strdup (zyre_event_peer_uuid (token));
        //  Forward token to all neighbors but father
        zlist_t *groups = zyre_own_groups (self->node);
        const char *group = (const char *) zlist_first (groups);
        while (group) {
            zlist_t *neighbors = zyre_peers_by_group (self->node, group);
            char *neighbor = (char *) zlist_first (neighbors);
            while (neighbor) {
                if (!streq (neighbor, self->father)) {
                    //  Send token to neighbor
                    zmsg_t *token = zmsg_new ();
                    zmsg_addstr (token, "tok");
                    zyre_whisper (self->node, neighbor, &token);
                    printf ("Forward to %s in group %s\n", neighbor, group);
                }
                //  Get next item in list
                neighbor = (char *) zlist_next (neighbors);
            }

            group = (const char *) zlist_next (groups);
            zlist_destroy (&neighbors);
        }
        zlist_destroy (&groups);
    }

    if (self->recv_msg == s_zecho_neighbor_count(self)) {
        if (streq (self->father, "initiator")) {
            //  Decide
            printf("Decide\n");
        }
        else {
            //  Send token to father
            zmsg_t *token = zmsg_new ();
            zmsg_addstr (token, "tok");
            zyre_whisper (self->node, self->father, &token);
            printf("Send to father\n");
        }
    }

    zyre_event_destroy (&token);
}


//  --------------------------------------------------------------------------
//  Initiate the echo algorithm

void
zecho_print (zecho_t *self) {
    printf ("zecho : {\n");
    printf ("    ID: %s,\n", zyre_uuid (self->node));
    printf ("    count: %d\n", self->recv_msg);
    printf ("    father: %s\n", self->father);
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
zecho_test (bool verbose)
{
    printf (" * zecho: ");

    //  @selftest
    int rc;
    //  Init zyre nodes
    zyre_t *node1 = zyre_new ("node1");
    assert (node1);
    //  Set inproc endpoint for this node
    rc = zyre_set_endpoint (node1, "inproc://zyre-node1");
    assert (rc == 0);
    //  Set up gossip network for this node
    zyre_gossip_bind (node1, "inproc://gossip-hub");
    rc = zyre_start (node1);
    assert (rc == 0);

    zyre_t *node2 = zyre_new ("node2");
    assert (node2);
    //  Set inproc endpoint for this node
    rc = zyre_set_endpoint (node2, "inproc://zyre-node2");
    assert (rc == 0);
    //  Set up gossip network for this node
    zyre_gossip_connect (node2, "inproc://gossip-hub");
    rc = zyre_start (node2);
    assert (rc == 0);

    zyre_t *node3 = zyre_new ("node3");
    assert (node3);
    //  Set inproc endpoint for this node
    rc = zyre_set_endpoint (node3, "inproc://zyre-node3");
    assert (rc == 0);
    //  Set up gossip network for this node
    zyre_gossip_connect (node3, "inproc://gossip-hub");
    rc = zyre_start (node3);
    assert (rc == 0);

    //  Setup echo
    zecho_t *echo1 = zecho_new (node1);
    zecho_t *echo2 = zecho_new (node2);
    zecho_t *echo3 = zecho_new (node3);

    //  Join topology
    zyre_join (node1, "GLOBAL");
    zyre_join (node2, "GLOBAL");
    zyre_join (node2, "LOCAL");
    zyre_join (node3, "LOCAL");

    //  Give time for them to interconnect
    zclock_sleep (500);

    zyre_dump (node1);
    zclock_sleep (150);
    zyre_dump (node2);
    zclock_sleep (150);
    zyre_dump (node3);
    zclock_sleep (150);

    zecho_init (echo1);
    zclock_sleep (500);

    zyre_event_t *event = NULL;

    do {
        event = zyre_event_new (node2);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    zecho_recv (echo2, event);

    do {
        event = zyre_event_new (node3);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    zecho_recv (echo3, event);

    do {
        event = zyre_event_new (node2);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    zecho_recv (echo2, event);

    do {
        event = zyre_event_new (node1);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    zecho_recv (echo1, event);

    // Print result
    zecho_print (echo1);
    zecho_print (echo2);
    zecho_print (echo3);

    //  Cleanup
    zecho_destroy (&echo1);
    zecho_destroy (&echo2);
    zecho_destroy (&echo3);

    zyre_stop (node1);
    zyre_stop (node2);
    zyre_stop (node3);

    zyre_destroy (&node1);
    zyre_destroy (&node2);
    zyre_destroy (&node3);

    //  @end
    printf ("OK\n");
}
