/*  =========================================================================
    zelection - Holds an election with all connected peers

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of zlogger.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    zelection - Holds an election with all connected peers
@discuss
@end
*/

#include "zlog_classes.h"

//  Structure of our class

struct _zelection_t {
    char *caw;          //  Current active wave
    char *father;       //  Father in the current active wave
    unsigned int erec;  //  Number of received election messages
    unsigned int lrec;  //  Number of received leader messages
    bool state;         //  True if leader else false

    char *leader;       //  Leader identity

    zyre_t *node;       //  zyre handle (not owned!)
    zvector_t *clock;   //  vector clock handle (not owned!)
    bool verbose;       //  verbose logging?
};


//  --------------------------------------------------------------------------
//  Create a new zelection

zelection_t *
zelection_new (zyre_t *node)
{
    zelection_t *self = (zelection_t *) zmalloc (sizeof (zelection_t));
    assert (self);
    //  Initialize class properties here
    self->caw = NULL;
    self->father = NULL;
    self->erec = 0;
    self->lrec = 0;
    self->state = false;

    self->leader = NULL;

    self->node = node;
    self->clock = NULL;
    self->verbose = false;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zelection

void
zelection_destroy (zelection_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zelection_t *self = *self_p;
        //  Free class properties here
        zstr_free (&self->caw);
        zstr_free (&self->father);
        zstr_free (&self->leader);
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  Local helper functions

static unsigned long
s_neighbors_count (zelection_t *self)
{
    assert (self);
    unsigned long neighbors_count = 0;
    zlist_t *groups = zyre_own_groups (self->node);
    const char *group = (const char *) zlist_first (groups);
    while (group) {
        zlist_t *neighbors = zyre_peers_by_group (self->node, group);
        neighbors_count += zlist_size (neighbors);
        zlist_destroy (&neighbors);

        group = (const char *) zlist_next (groups);
    }
    zlist_destroy (&groups);
    return neighbors_count;
}


static zlist_t *
s_neighbors (zelection_t *self, bool with_father)
{
    assert (self);
    zlist_t *all_neighbors = zlist_new ();

    zlist_t *groups = zyre_own_groups (self->node);
    const char *group = (const char *) zlist_first (groups);
    while (group) {
        zlist_t *neighbors = zyre_peers_by_group (self->node, group);
        while (zlist_size (neighbors) > 0) {
            if (with_father)
                zlist_append (all_neighbors, zlist_pop (neighbors));
            else {
                char *neighbor = (char *) zlist_pop (neighbors);
                if (!streq (neighbor, self->father))
                    zlist_append (all_neighbors, neighbor);
                else
                    zstr_free (&neighbor);
            }
        }
        zlist_destroy (&neighbors);
        //  Get next item in list
        group = (const char *) zlist_next (groups);
    }

    zlist_destroy (&groups);
    //  Set autofree to properly free all items. Has to happen only now
    //  otherwise zlist_append() will take a copy.
    zlist_autofree (all_neighbors);
    return all_neighbors;
}

static void
s_send_to (zelection_t *self, zmsg_t *msg, zlist_t *peers)
{
    assert (self);
    assert (msg);
    assert (peers);

    const char *peer = (const char *) zlist_first (peers);
    while (peer) {
        //  Send message to peer
        zmsg_t *copy = zmsg_dup (msg);
        if (self->clock)
            zvector_send_prepare (self->clock, copy);

        zyre_whisper (self->node, peer, &copy);

        //  Get next peer in list
        peer = (char *) zlist_next (peers);
    }

    zlist_destroy (&peers);
    zmsg_destroy (&msg);
}

//  --------------------------------------------------------------------------
//  Initiate election

void
zelection_start (zelection_t *self)
{
    assert (self);
    self->caw = strdup (zyre_uuid (self->node));

    zmsg_t *election_msg = zmsg_new ();
    zmsg_addstr (election_msg, "ZLE");
    zmsg_addstr (election_msg, "ELECTION");
    zmsg_addstr (election_msg, zyre_uuid (self->node));

    //  Send election message to all neighbors
    s_send_to (self, election_msg, s_neighbors (self, true));
    if (self->verbose)
        zsys_info ("ELECTION started by %s\n", zyre_uuid (self->node));
}


//  --------------------------------------------------------------------------
//  Handle received election and leader messages. Return 1 if election is
//  still in progress, 0 if election is concluded and -1 is an error occurred.

int
zelection_recv (zelection_t *self, zyre_event_t *event)
{
    assert (self);
    assert (event);

    zmsg_t *msg = zyre_event_msg (event);
    char *type = zmsg_popstr (msg);
    char *r = zmsg_popstr (msg);

    if (streq (type, "ELECTION")) {
        //  Initiate or re-initiate leader election
        if (!self->caw || strcmp (r, self->caw) < 0) {
            zstr_free (&self->caw);     //  Free caw when re-initiated
            zstr_free (&self->father);  //  Free father when re-initiated
            self->caw = strdup (r);
            self->erec = 0;
            self->father = strdup (zyre_event_peer_uuid (event));

            zmsg_t *election_msg = zmsg_new ();
            zmsg_addstr (election_msg, "ZLE");
            zmsg_addstr (election_msg, "ELECTION");
            zmsg_addstr (election_msg, r);

            //  Send election message to all neighbors but father but father
            s_send_to (self, election_msg, s_neighbors (self, false));
            if (self->verbose)
                zsys_info ("Initialise election %s\n", zyre_uuid (self->node));
        }

        //  Participate in current active wave
        if (strcmp (r, self->caw) == 0) {
            self->erec++;
            if (self->erec == s_neighbors_count (self)) {
                if (streq (self->caw, zyre_uuid (self->node))) {
                    zmsg_t *leader_msg = zmsg_new ();
                    zmsg_addstr (leader_msg, "ZLE");
                    zmsg_addstr (leader_msg, "LEADER");
                    zmsg_addstr (leader_msg, r);

                    //  Send leader message to all neighbors
                    s_send_to (self, leader_msg, s_neighbors (self, true));
                    if (self->verbose)
                        zsys_info ("LEADER decision by %s\n", zyre_uuid (self->node));
                }
                else {
                    zmsg_t *election_msg = zmsg_new ();
                    zmsg_addstr (election_msg, "ZLE");
                    zmsg_addstr (election_msg, "ELECTION");
                    zmsg_addstr (election_msg, self->caw);

                    if (self->clock)
                        zvector_send_prepare (self->clock, election_msg);

                    //  Send election message to father
                    zyre_whisper (self->node, self->father, &election_msg);
                    if (self->verbose)
                        zsys_info ("Echo wave to father %s\n", zyre_uuid (self->node));
                }
            }
        }
        //  If r > caw, the message is ignored!
    }
    else
    if (streq(type, "LEADER")) {
        if (self->lrec == 0) {
            zmsg_t *leader_msg = zmsg_new ();
            zmsg_addstr (leader_msg, "ZLE");
            zmsg_addstr (leader_msg, "LEADER");
            zmsg_addstr (leader_msg, r);

            //  Send leader message to all neighbors
            s_send_to (self, leader_msg, s_neighbors (self, true));
            if (self->verbose)
                zsys_info ("Propagate LEADER by %s\n", zyre_uuid (self->node));
        }
        self->lrec++;
        zstr_free (&self->leader);
        self->leader = strdup (r);
    }

    zstr_free (&type);
    zstr_free (&r);
    zyre_event_destroy (&event);

    if (self->lrec == s_neighbors_count (self)) {
        self->state = streq (self->leader, zyre_uuid (self->node));
        if (self->verbose)
            zsys_info ("Election finished %s, %s!\n", zyre_uuid (self->node), self->state? "true": "false");
        return 0;
    }
    else
        return 1;
}


//  --------------------------------------------------------------------------
//  Set a vector clock handle. Election message will be prepended with the
//  vector if not NULL.

void
zelection_set_clock (zelection_t *self, zvector_t *clock)
{
    assert (self);
    self->clock = clock;
}


//  --------------------------------------------------------------------------
//  Enable/disable verbose logging.

void
zelection_set_verbose (zelection_t *self, bool verbose)
{
    assert (self);
    self->verbose = verbose;
}


//  --------------------------------------------------------------------------
//  Print election status to command line

void
zelection_print (zelection_t *self) {
    printf ("zelection : {\n");
    printf ("    ID: %s,\n", zyre_uuid (self->node));
    printf ("    father: %s\n", self->father);
    printf ("    CAW: %s\n", self->caw);
    printf ("    election count: %d\n", self->erec);
    printf ("    leader count: %d\n", self->lrec);
    printf ("    state: %s\n", !self->leader? "undecided": self->state? "leader": "looser");
    printf ("    leader: %s\n", self->leader);
    printf ("}\n");
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
zelection_test (bool verbose)
{
    printf (" * zelection: ");
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

    //  Join topology
    zyre_join (node1, "GLOBAL");
    zyre_join (node2, "GLOBAL");

    //  Give time for them to interconnect
    zclock_sleep (500);

    //  Setup zelection
    zelection_t *node1_election = zelection_new (node1);
    zelection_t *node2_election = zelection_new (node2);
    assert (node1_election);
    assert (node2_election);
    zelection_set_verbose (node1_election, verbose);
    zelection_set_verbose (node2_election, verbose);

    zelection_start (node1_election);

    zyre_event_t *event = NULL;
    do {
        event = zyre_event_new (node2);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    char *type = zmsg_popstr (zyre_event_msg (event));
    assert (streq (type, "ZLE"));
    zstr_free (&type);
    rc = zelection_recv (node2_election, event);
    assert (rc == 1);

    do {
        event = zyre_event_new (node1);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    type = zmsg_popstr (zyre_event_msg (event));
    assert (streq (type, "ZLE"));
    zstr_free (&type);
    rc = zelection_recv (node1_election, event);
    assert (rc == 1);

    do {
        event = zyre_event_new (node2);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    type = zmsg_popstr (zyre_event_msg (event));
    assert (streq (type, "ZLE"));
    zstr_free (&type);
    rc = zelection_recv (node2_election, event);
    assert (rc == 0);

    do {
        event = zyre_event_new (node1);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    type = zmsg_popstr (zyre_event_msg (event));
    assert (streq (type, "ZLE"));
    zstr_free (&type);
    rc = zelection_recv (node1_election, event);
    assert (rc == 0);

    //  Cleanup
    zelection_destroy (&node1_election);
    zelection_destroy (&node2_election);

    zyre_stop (node1);
    zyre_stop (node2);

    zyre_destroy (&node1);
    zyre_destroy (&node2);
    //  @end
    printf ("OK\n");
}
