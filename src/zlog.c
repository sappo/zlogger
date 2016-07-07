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

static zvector_t *
s_get_zvector_from_logMsg (char *logMsg);

//static int
//s_get_timestamp_from_logMsg (char *logMsg);

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


//  Extracts the vectorclock from a given logMsg and returns as new vector

static zvector_t *
s_get_zvector_from_logMsg (char *logMsg)
{
  assert (logMsg);

  char *log = strdup (logMsg);
  char *clock_ptr = NULL;
  clock_ptr = strtok(log, "/");
  clock_ptr = strtok(NULL, "/");

  zvector_t *ret = zvector_from_string(clock_ptr);
  zstr_free (&log);

  return ret;
}

//  Extracts the timestamo from a given logMsg and returns as int
/*
static int
s_get_timestamp_from_logMsg (char *logMsg)
{
  assert (logMsg);
  return 1;
}
*/

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
//  Compares the zvector_t's of given logMsg a to logMsg b.
//  Returns -1 if a < b, 0 if a = b, 1 if a > b and 2 if a and b parallel

int
zlog_compare_logMsg (const char *logMsg_a, const char *logMsg_b)
{
  zvector_t *logMsg_a_vector = s_get_zvector_from_logMsg ((char *)logMsg_a);
  zvector_t *logMsg_b_vector = s_get_zvector_from_logMsg ((char *)logMsg_b);

  int ret = zvector_compare_to (logMsg_a_vector, logMsg_b_vector);
  zvector_destroy (&logMsg_a_vector);
  zvector_destroy (&logMsg_b_vector);

  return ret;
}

//  --------------------------------------------------------------------------
//  Orders log with the given filepath

void
zlog_order_log (const char *path_src, const char *path_dst)
{
  assert (path_src);
  assert (path_dst);

  zfile_t *file_src = zfile_new (NULL, path_src);
  FILE *file_dst = fopen(path_dst, "w");
  assert (file_src);
  assert (file_dst);

  zfile_input (file_src);
  zlistx_t *ordered_list = zlistx_new ();
  zlistx_set_destructor (ordered_list, (zlistx_destructor_fn *) zstr_free);
  zlistx_set_duplicator (ordered_list, (zlistx_duplicator_fn *) strdup);
  zlistx_set_comparator (ordered_list, (zlistx_comparator_fn *) zlog_compare_logMsg);

  // Insert data in a ordered_list and order it with given compare function
  const char *line = zfile_readln (file_src);
  while (line != NULL) {
    zlistx_insert (ordered_list, (void *)line, true);
    line = zfile_readln (file_src);
  }

  // write ordered data from ordered_list in a new logfile
  const char newLineSymbol = '\n';
  line = (const char *) zlistx_first (ordered_list);
  while (line != NULL) {
    fwrite (line, 1, strlen (line), file_dst);
    fwrite (&newLineSymbol, 1, 1, file_dst);
    //printf("%s %lu\n", line, strlen (line));
    line = (const char *) zlistx_next (ordered_list);
  }

  zlistx_destroy (&ordered_list);
  zfile_destroy (&file_src);
  fclose (file_dst);
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


    //zlog_order_log ("/var/log/vc.log", "ordered_vc1.log");
    //  @end

    printf ("OK\n");
}
