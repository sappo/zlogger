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
    zecho - Implements the echo algorithms which consist of two distinct waves.
            The first wave is to flood the network and build a spanning tree. It
            can be used to inform peers thus it's also called inform wave. The
            second wave is flows from the leaves of the spanning tree back to the
            initiator. It it used to collect things from peers. Collectables are
            e.g. ACKs or arbitrary data.
@discuss
@end
*/

#include "zlog_classes.h"

//  Structure of our class

struct _zecho_t {
    unsigned int recv_msg;      //  Counts the number of received messages.
    char *father;               //  Father in the echo wave
    char *wave_id;              //  Id of the current wave

    void *inform_handler;                   //  Inform handler object
    zecho_process_fn *inform_process_fn;    //  Process inform messages
    zecho_create_fn *inform_create_fn;      //  Create own inform message
    void *collect_handler;                  //  Collect handler object
    zecho_process_fn *collect_process_fn;   //  Process collect messages
    zecho_create_fn *collect_create_fn;     //  Create own collect message

    zyre_t *node;       //  Own zyre handle (not owned!)
    zvector_t *clock;   //  vector clock handle (not owned!)
    bool verbose;       //  verbose logging?
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
    self->wave_id = NULL;
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
        zstr_free (&self->wave_id);
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Initiate the echo algorithm

void
zecho_init (zecho_t *self)
{
    assert (self);
    self->father = strdup("initiator");
    self->wave_id = strdup (zyre_uuid (self->node));

    zlist_t *groups = zyre_own_groups (self->node);
    const char *group = (const char *) zlist_first (groups);
    while (group) {
        zlist_t *neighbors = zyre_peers_by_group (self->node, group);
        if (self->verbose)
            zsys_info ("Send to group: %s (%lu)\n", group, zlist_size (neighbors));

        char *neighbor = (char *) zlist_first (neighbors);
        while (neighbor) {
            if (!streq (neighbor, self->father)) {
                //  Send token to neighbor
                zmsg_t *inform_msg = zmsg_new ();
                zmsg_addstr (inform_msg, "ZECHO");
                zmsg_addstr (inform_msg, self->wave_id);
                zmsg_addstr (inform_msg, "INFORM");
                //  Get inform message from handler
                if (self->inform_create_fn) {
                    zmsg_t *handler_msg = self->inform_create_fn (self, self->inform_handler);
                    zmsg_addmsg (inform_msg, &handler_msg);
                }
                if (self->clock)
                    zvector_send_prepare (self->clock, inform_msg);

                //  Send INFORM message to neighbor
                zyre_whisper (self->node, neighbor, &inform_msg);
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
    assert (self);
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

int
zecho_recv (zecho_t *self, zyre_event_t *token)
{
    assert (self);
    assert (token);
    self->recv_msg++;

    char *wave_id = zmsg_popstr (zyre_event_msg (token));
    if (self->father && !streq (self->wave_id, wave_id)) {
        zstr_free (&wave_id);
        zyre_event_destroy (&token);
        return -1;     //  Wrong wave
    }

    char *wave_direction = zmsg_popstr (zyre_event_msg (token));
    if (!self->father) {
        self->father = strdup (zyre_event_peer_uuid (token));
        self->wave_id = wave_id;
        //  Forward token to all neighbors but father
        zlist_t *groups = zyre_own_groups (self->node);
        const char *group = (const char *) zlist_first (groups);
        while (group) {
            zlist_t *neighbors = zyre_peers_by_group (self->node, group);
            char *neighbor = (char *) zlist_first (neighbors);
            while (neighbor) {
                if (!streq (neighbor, self->father)) {
                    zmsg_t *inform_msg = zmsg_new ();
                    zmsg_addstr (inform_msg, "ZECHO");
                    zmsg_addstr (inform_msg, self->wave_id);
                    zmsg_addstr (inform_msg, "INFORM");
                    //  Process inform message
                    if (self->inform_process_fn) {
                        zmsg_t *msg = zyre_event_msg (token);
                        zmsg_t *popmsg = zmsg_popmsg (msg);
                        self->inform_process_fn (self, popmsg, self->inform_handler);
                    }

                    //  Get inform message from handler
                    if (self->inform_create_fn) {
                        zmsg_t *handler_msg = self->inform_create_fn (self, self->inform_handler);
                        zmsg_addmsg (inform_msg, &handler_msg);
                    }
                    if (self->clock)
                        zvector_send_prepare (self->clock, inform_msg);

                    //  Send INFORM message to neighbor
                    zyre_whisper (self->node, neighbor, &inform_msg);
                    if (self->verbose)
                        zsys_info ("Forward to %s in group %s\n", neighbor, group);

                }
                //  Get next item in list
                neighbor = (char *) zlist_next (neighbors);
            }

            group = (const char *) zlist_next (groups);
            zlist_destroy (&neighbors);
        }
        zlist_destroy (&groups);
        zyre_event_destroy (&token);
    }
    else
        zstr_free (&wave_id);

    if (self->recv_msg == s_zecho_neighbor_count (self)) {
        if (streq (self->father, "initiator")) {
            //  Decide
            if (token && self->collect_process_fn) {
                zmsg_t *msg = zyre_event_msg (token);
                self->collect_process_fn (self, zmsg_popmsg (msg), self->collect_handler);
            }

            if (self->verbose)
                zsys_info ("Decide\n");
        }
        else {
            zmsg_t *collect_msg = zmsg_new ();
            zmsg_addstr (collect_msg, "ZECHO");
            zmsg_addstr (collect_msg, self->wave_id);
            zmsg_addstr (collect_msg, "COLLECT");
            //  Process message from peer
            if (token) {
                if (streq (wave_direction, "INFORM")) {
                    if (self->inform_process_fn) {
                        zmsg_t *msg = zyre_event_msg (token);
                        zmsg_t *popmsg = zmsg_popmsg (msg);
                        self->inform_process_fn (self, popmsg, self->inform_handler);
                    }
                }
                else
                if (streq (wave_direction, "COLLECT")) {
                    if (self->collect_process_fn) {
                        zmsg_t *msg = zyre_event_msg (token);
                        zmsg_t *popmsg = zmsg_popmsg (msg);
                        self->collect_process_fn (self, popmsg, self->collect_handler);
                    }
                }
            }

            //  Get collect message from handler
            if (self->collect_create_fn) {
                zmsg_t *handler_msg = self->collect_create_fn (self, self->collect_handler);
                zmsg_addmsg (collect_msg, &handler_msg);
            }
            if (self->clock)
                zvector_send_prepare (self->clock, collect_msg);

            //  Send COLLECT message to father
            zyre_whisper (self->node, self->father, &collect_msg);
            if (self->verbose)
                zsys_info ("Send to father\n");

        }
        zyre_event_destroy (&token);
        zstr_free (&wave_direction);
        return 1;
    }
    else
    if (streq (wave_direction, "COLLECT")) {
        //  Process collect message from peer
        if (self->collect_process_fn) {
            zmsg_t *msg = zyre_event_msg (token);
            zmsg_t *popmsg = zmsg_popmsg (msg);
            self->collect_process_fn (self, popmsg, self->collect_handler);
        }

        if (self->verbose)
            zsys_info ("Received from peer\n");

    }
    zyre_event_destroy (&token);
    zstr_free (&wave_direction);
    return 0;
}


//  --------------------------------------------------------------------------
//  Get wave id

const char *
zecho_wave_id (zecho_t *self)
{
    assert (self);
    return self->wave_id;
}


//  --------------------------------------------------------------------------
//  Sets a handler which is passed to custom collect functions.

void
zecho_set_collect_handler (zecho_t *self, void *handler)
{
    assert (self);
    self->collect_handler = handler;
}


//  --------------------------------------------------------------------------
//  Set a user-defined function to process collect messages from peers; This
//  function is invoked during the second (incoming) wave.

void
zecho_set_collect_process (zecho_t *self, zecho_process_fn *process_fn)
{
    assert (self);
    self->collect_process_fn = process_fn;
}


//  --------------------------------------------------------------------------
//  Set a user-defined function to create a custom message part for the collect
//  message; This function is invoked during the second (incoming) wave. The
//  returned message's content is appended to the wave message.

void
zecho_set_collect_create (zecho_t *self, zecho_create_fn *collect_fn)
{
    assert (self);
    self->collect_create_fn = collect_fn;
}


//  --------------------------------------------------------------------------
//  Sets a handler which is passed to custom inform functions.

void
zecho_set_inform_handler (zecho_t *self, void *handler)
{
    assert (self);
    self->inform_handler = handler;
}


//  --------------------------------------------------------------------------
//  Set a user-defined function to process inform messages from peers; This
//  function is invoked during the first (outgoing) wave.

void
zecho_set_inform_process (zecho_t *self, zecho_process_fn *process_fn)
{
    assert (self);
    self->inform_process_fn = process_fn;
}


//  --------------------------------------------------------------------------
//  Set a user-defined function to create a custom message part for the inform
//  message; This function is invoked during the first (outgoing) wave. The
//  returned message's content is appended to the wave message.

void
zecho_set_inform_create (zecho_t *self, zecho_create_fn *collect_fn)
{
    assert (self);
    self->inform_create_fn = collect_fn;
}


//  --------------------------------------------------------------------------
//  Set a vector clock handle. Echo messages will be prepended with the
//  vector if not NULL.

void
zecho_set_clock (zecho_t *self, zvector_t *clock)
{
    assert (self);
    self->clock = clock;
}


//  --------------------------------------------------------------------------
//  Enable/disable verbose logging.

void
zecho_set_verbose (zecho_t *self, bool verbose)
{
    assert (self);
    self->verbose = verbose;
}


//  --------------------------------------------------------------------------
//  Print echo status to command line

void
zecho_print (zecho_t *self) {
    printf ("zecho : {\n");
    printf ("    ID: %s,\n", zyre_uuid (self->node));
    printf ("    count: %d\n", self->recv_msg);
    printf ("    father: %s\n", self->father);
    printf ("    wave id: %s\n", self->wave_id);
    printf ("}\n");
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
s_test_zecho_process (zecho_t *self, zmsg_t *msg,  void *handler)
{
    assert (self);
    char *str = zmsg_popstr (msg);
    assert (streq (str, "blub"));
    zstr_free (&str);
    zmsg_destroy (&msg);
}

zmsg_t *
s_test_zecho_create (zecho_t *self, void *handler)
{
    assert (self);
    zmsg_t *msg = zmsg_new ();
    zmsg_addstr (msg, "blub");
    return msg;
}

void
zecho_test (bool verbose)
{
    printf (" * zecho: ");
    if (verbose)
        printf ("\n");

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
    zecho_set_verbose (echo1, verbose);
    zecho_set_verbose (echo2, verbose);
    zecho_set_verbose (echo3, verbose);
    zecho_set_collect_process (echo1, s_test_zecho_process);
    zecho_set_collect_process (echo2, s_test_zecho_process);
    zecho_set_collect_process (echo3, s_test_zecho_process);
    zecho_set_collect_create (echo1, s_test_zecho_create);
    zecho_set_collect_create (echo2, s_test_zecho_create);
    zecho_set_collect_create (echo3, s_test_zecho_create);

    //  Join topology
    zyre_join (node1, "GLOBAL");
    zyre_join (node2, "GLOBAL");
    zyre_join (node2, "LOCAL");
    zyre_join (node3, "LOCAL");

    //  Give time for them to interconnect
    zclock_sleep (500);

    if (verbose) {
        zyre_dump (node1);
        zclock_sleep (50);
        zyre_dump (node2);
        zclock_sleep (50);
        zyre_dump (node3);
        zclock_sleep (50);
    }

    zecho_init (echo1);

    zyre_event_t *event = NULL;

    do {
        event = zyre_event_new (node2);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    char *type = zmsg_popstr (zyre_event_msg (event));
    assert (streq (type, "ZECHO"));
    zstr_free (&type);
    zecho_recv (echo2, event);

    do {
        event = zyre_event_new (node3);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    type = zmsg_popstr (zyre_event_msg (event));
    assert (streq (type, "ZECHO"));
    zstr_free (&type);
    zecho_recv (echo3, event);

    do {
        event = zyre_event_new (node2);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    type = zmsg_popstr (zyre_event_msg (event));
    assert (streq (type, "ZECHO"));
    zstr_free (&type);
    zecho_recv (echo2, event);

    do {
        event = zyre_event_new (node1);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    type = zmsg_popstr (zyre_event_msg (event));
    assert (streq (type, "ZECHO"));
    zstr_free (&type);
    zecho_recv (echo1, event);

    if (verbose) {
        // Print result
        zecho_print (echo1);
        zecho_print (echo2);
        zecho_print (echo3);
    }

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
