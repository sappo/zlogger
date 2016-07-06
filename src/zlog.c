/*  =========================================================================
    zlog - zlog actor

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of zlogger.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    zlog - zlog actor
@discuss
@end
*/

#include "zlog_classes.h"

//  Structure of our actor

struct _zlog_t {
    zsock_t *pipe;              //  Actor command pipe
    zloop_t *loop;              //  Actor event loop
    bool terminated;            //  Did caller ask us to quit?
    bool verbose;               //  Verbose logging enabled?
    //  Actor properties
    zelection_t *election;      //  Election mechanism
    zvector_t *clock;           //  Vector clock for this self
    zyre_t *node;               //  Zyre handle
};


//  --------------------------------------------------------------------------
//  Internal helper functions

static int
s_zlog_recv_api (zloop_t *loop, zsock_t *reader, void *arg);

static int
s_zlog_recv_zyre (zloop_t *loop, zsock_t *reader, void *arg);

//  --------------------------------------------------------------------------
//  Create a new zlog instance

static zlog_t *
zlog_new (zsock_t *pipe, void *args)
{
    char **params = (char **) args;
    assert (params);

    zlog_t *self = (zlog_t *) zmalloc (sizeof (zlog_t));
    assert (self);

    self->pipe = pipe;
    self->terminated = false;
    self->loop = zloop_new ();
    zloop_reader (self->loop, self->pipe, s_zlog_recv_api, self);

    //  Initialize properties
    char *name = params[1];
    assert (name);
    self->node = zyre_new (name);
    zloop_reader (self->loop, zyre_socket (self->node), s_zlog_recv_zyre, self);
    self->clock = zvector_new (zyre_uuid (self->node));
    self->election = zelection_new (self->node);
    zelection_set_clock (self->election, self->clock);

    //  Set node endpoint
    char *endpoint = params[0];
    assert (endpoint);
    zyre_set_endpoint (self->node, "%s", endpoint);

    //  Set gossip discovery algorithm
    if (streq (params[2], "GOSSIP MASTER"))
        zyre_gossip_bind (self->node, "inproc://gossip-hub");
    else
    if (streq (params[2], "GOSSIP SLAVE"))
        zyre_gossip_connect (self->node, "inproc://gossip-hub");

    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zlog instance

static void
zlog_destroy (zlog_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zlog_t *self = *self_p;

        //  Free actor properties
        zvector_destroy (&self->clock);
        zelection_destroy (&self->election);
        zyre_destroy (&self->node);

        //  Free object itself
        zloop_destroy (&self->loop);
        free (self);
        *self_p = NULL;
    }
}


//  Start this actor. Return a value greater or equal to zero if initialization
//  was successful. Otherwise -1.

static int
zlog_start (zlog_t *self)
{
    assert (self);

    //  Startup actions
    int rc = zyre_start (self->node);
    assert (rc == 0);
    rc = zyre_join (self->node, "GLOBAL");
    assert (rc == 0);

    //  Give time to interconnect
    zclock_sleep (250);

    zelection_start (self->election);

    return rc;
}


//  Stop this actor. Return a value greater or equal to zero if stopping
//  was successful. Otherwise -1.

static int
zlog_stop (zlog_t *self)
{
    assert (self);

    //  Shutdown actions
    zyre_stop (self->node);

    return 0;
}


//  Here we handle incoming message from the node

static int
s_zlog_recv_api (zloop_t *loop, zsock_t *reader, void *arg)
{
    assert (arg);
    zlog_t *self = (zlog_t *) arg;

    //  Get the whole message of the pipe in one go
    zmsg_t *request = zmsg_recv (self->pipe);
    if (!request)
       return 0;        //  Interrupted, gracefully deny error. Keep going!

    char *command = zmsg_popstr (request);
    if (streq (command, "START"))
        zlog_start (self);
    else
    if (streq (command, "STOP"))
        zlog_stop (self);
    else
    if (streq (command, "VERBOSE")) {
        self->verbose = true;
        zelection_set_verbose (self->election, true);
    }
    else
    if (streq (command, "$TERM"))
        //  The $TERM command is send by zactor_destroy() method
        self->terminated = true;
    else {
        zsys_error ("invalid command '%s'", command);
        assert (false);
    }
    zstr_free (&command);
    zmsg_destroy (&request);

    //  Negative return value will abort loop!
    return self->terminated? -1: 0;
}


//  Here we handle incoming message from zyre

static int
s_zlog_recv_zyre (zloop_t *loop, zsock_t *reader, void *arg)
{
    assert (arg);
    zlog_t *self = (zlog_t *) arg;

    //  Get the whole message of the pipe in one go
    zyre_event_t *event = zyre_event_new (self->node);
    if (!event)
       return -1;        //  Interrupted, stop zyre processing!

    const char *type = zyre_event_type (event);
    if (streq (type, "WHISPER")) {
        zmsg_t *request = zyre_event_msg (event);
        zvector_recv (self->clock, request);
        char *command = zmsg_popstr (request);
        //  Handle election messages
        if (streq (command, "ZLE")) {
            int rc = zelection_recv (self->election, event);
            if (rc == 0) {
                if (self->verbose)
                    zelection_print (self->election);

                //  TODO: leader action
            }
            //  rc == -1, will be ignored! We just let the election starve.
        }
        zstr_free (&command);
    }
    else
        zyre_event_destroy (&event);

    return 0;
}


//  --------------------------------------------------------------------------
//  This is the actor which runs in its own thread.

void
zlog_actor (zsock_t *pipe, void *args)
{
    zlog_t * self = zlog_new (pipe, args);
    if (!self)
        return;          //  Interrupted

    //  Signal actor successfully initiated
    zsock_signal (self->pipe, 0);

    zloop_start (self->loop);

    zlog_destroy (&self);
}

//  --------------------------------------------------------------------------
//  Self test of this actor.

void
zlog_test (bool verbose)
{
    printf (" * zlog: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    char *params1[3] = {"inproc://logger1", "logger1", "GOSSIP MASTER"};
    zactor_t *zlog = zactor_new (zlog_actor, params1);

    char *params2[3] = {"inproc://logger2", "logger2", "GOSSIP SLAVE"};
    zactor_t *zlog2 = zactor_new (zlog_actor, params2);

    char *params3[3] = {"inproc://logger3", "logger3", "GOSSIP SLAVE"};
    zactor_t *zlog3 = zactor_new (zlog_actor, params3);

    char *params4[3] = {"inproc://logger4", "logger4", "GOSSIP SLAVE"};
    zactor_t *zlog4 = zactor_new (zlog_actor, params4);

    if (verbose) {
        zstr_send (zlog, "VERBOSE");
        zstr_send (zlog2, "VERBOSE");
        zstr_send (zlog3, "VERBOSE");
        zstr_send (zlog4, "VERBOSE");
    }

    zstr_send (zlog, "START");
    zstr_send (zlog2, "START");
    zstr_send (zlog3, "START");
    zstr_send (zlog4, "START");

    //  Give time to interconnect and elect
    zclock_sleep (750);

    zstr_send (zlog, "STOP");
    zstr_send (zlog2, "STOP");
    zstr_send (zlog3, "STOP");
    zstr_send (zlog4, "STOP");

    //  Give time to disconnect
    zclock_sleep (250);

    zactor_destroy (&zlog);
    zactor_destroy (&zlog2);
    zactor_destroy (&zlog3);
    zactor_destroy (&zlog4);
    //  @end

    printf ("OK\n");
}
