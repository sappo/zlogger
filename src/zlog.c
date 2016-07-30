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
    bool dump_ts;               //  Dump time space subgraph during destruction

    //  Leader properties
    int leader_timer;           //  ID of leader's collect timer
    zlistx_t *ordered_log;      //  List of ordered log entries
    //  Peer properties
    zlistx_t *collect_log;      //  Collect log messages from peers to forward to father
    int linesRead;              //  How many lines have been read from logfile
    //  Communication properties
    zelection_t *election;      //  Election mechanism
    zecho_t *collector;         //  Log collector
    zvector_t *clock;           //  Vector clock for this self

    zyre_t *node;               //  Zyre handle
};


//  --------------------------------------------------------------------------
//  Internal helper functions

static zvector_t *
s_get_clock_from_log_msg (char *logMsg);

static unsigned long long *
s_get_timestamp_from_logMsg (char *logMsg);

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

    zlog_t *self = (zlog_t *) zmalloc (sizeof (zlog_t));
    assert (self);

    self->pipe = pipe;
    self->terminated = false;
    self->loop = zloop_new ();
    zloop_reader (self->loop, self->pipe, s_zlog_recv_api, self);
    zsys_set_logsystem (true);

    //  Initialize properties
    self->node = zyre_new (NULL);
    zloop_reader (self->loop, zyre_socket (self->node), s_zlog_recv_zyre, self);
    self->clock = zvector_new (zyre_uuid (self->node));
    self->election = zelection_new (self->node);
    zelection_set_clock (self->election, self->clock);
    self->dump_ts = false;

    //  Initialize leader properties
    self->ordered_log = zlistx_new ();
    zlistx_set_destructor (self->ordered_log, (zlistx_destructor_fn *) zstr_free);
    zlistx_set_comparator (self->ordered_log, (zlistx_comparator_fn *) zlog_compare_log_msg_vc);

    //  Initialize peer properties
    self->collect_log = zlistx_new ();
    zlistx_set_destructor (self->collect_log, (zlistx_destructor_fn *) zstr_free);
    self->linesRead = 0;

    //  Enable Gossip discovery
    if (params) {
        //  Set node endpoint
        char *endpoint = params[0];
        zyre_set_endpoint (self->node, "%s", endpoint);

        //  Set gossip discovery algorithm
        if (streq (params[1], "GOSSIP MASTER"))
            zyre_gossip_bind (self->node, "inproc://gossip-hub");
        else
        if (streq (params[1], "GOSSIP SLAVE"))
            zyre_gossip_connect (self->node, "inproc://gossip-hub");
    }

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

        if (self->dump_ts)
            zvector_dump_time_space (self->clock);

        //  Free actor properties
        zvector_destroy (&self->clock);
        zelection_destroy (&self->election);
        zecho_destroy (&self->collector);
        zyre_destroy (&self->node);
        zlistx_destroy (&self->ordered_log);
        zlistx_destroy (&self->collect_log);

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
s_get_clock_from_log_msg (char *logMsg)
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

//  Extracts the timestamp from a given logMsg

static unsigned long long *
s_get_timestamp_from_logMsg (char *logMsg)
{
  assert (logMsg);

  char *log = strdup (logMsg);
  char *ts_ptr = NULL;
  ts_ptr = strtok(log, " ");
  unsigned long long *ts_val = (unsigned long long *) zmalloc (sizeof (unsigned long long));
  *ts_val = strtoull(ts_ptr, NULL, 10);

  zstr_free (&log);

  return ts_val;
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
    if (streq (command, "SEND RANDOM")) {
        char *content = zmsg_popstr (request);
        char *owner = zmsg_popstr (request);

        zlist_t *peers = zyre_peers (self->node);
        if (!peers || zlist_size (peers) == 0)
            zvector_info (self->clock, "%s", "No friends!");
        else {
            zvector_info (self->clock, "S: %s - %.5s", content, owner);
            zmsg_t *msg = zmsg_new ();
            zvector_send_prepare (self->clock, msg);
            zmsg_addstr (msg, "BAKERY");
            zmsg_addstr (msg, content);
            if (!owner)
                owner = strdup (zyre_uuid (self->node));

            zmsg_addstr (msg, owner);

            int rand = randof (zlist_size (peers));
            int index = 0;
            const char *peer = (const char *) zlist_first (peers);
            for (index = 0; index <= rand; index++) {
                if (rand == index)
                    zyre_whisper (self->node, peer, &msg);

                peer =  (const char *) zlist_next (peers);
            }
        }

        zlist_destroy (&peers);
        zstr_free (&content);
        zstr_free (&owner);
    }
    else
    if (streq (command, "DUMP TS"))
        self->dump_ts = true;
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


static void
s_zlog_process_collect_log (zecho_t *echo, zmsg_t *msg, zlog_t *self)
{
    assert (self);

    if (zelection_won (self->election)) {
        /*printf ("LEADER\n");*/
        //  Read log message and order log
        if (self->verbose)
            zvector_info (self->clock, "Order received logs %s\n", zyre_uuid (self->node));

        char *logmsg = zmsg_popstr (msg);
        while (logmsg) {
            zlistx_insert (self->ordered_log, logmsg, true);
            logmsg = zmsg_popstr (msg);
        }

        FILE *logfile = fopen("./ordered_log", "w+");
        logmsg = (char *) zlistx_first (self->ordered_log);
        while (logmsg) {
            fprintf (logfile, "%s\n", logmsg);
            //  Next log message
            logmsg = (char *) zlistx_next (self->ordered_log);
        }
        fclose (logfile);
    }
    else {
        /*printf ("SLAVE\n");*/
        //  Save collect log messages from peers
        char *logmsg = zmsg_popstr (msg);
        while (logmsg) {
            zlistx_insert (self->collect_log, logmsg, false);
            logmsg = zmsg_popstr (msg);
        }
    }
    zmsg_destroy (&msg);
}


static zlistx_t *
s_zlog_read_log (zlog_t *self)
{
    assert (self);
    zlistx_t *messages = zlistx_new ();
    zlistx_set_duplicator (messages, (zlistx_duplicator_fn *) strdup);

    //  Read own log file
    char *filename = zsys_sprintf ("vc_%s.log", zyre_uuid (self->node));
    zfile_t *logfile = zfile_new ("/tmp", filename);
    //  Read all log entries
    if (!zfile_is_readable (logfile))
        goto cleanup;           //  No log entries yet

    /*printf ("Read %s\n", zfile_filename (logfile, NULL));*/
    zfile_input (logfile);
    int cnt = 1;
    const char *logmsg = zfile_readln (logfile);
    while (logmsg) {
        if (cnt > self->linesRead) {
            zlistx_insert (messages, (char *) logmsg, false);
            self->linesRead++;
        }
        logmsg = zfile_readln (logfile);
        cnt++;
    }

cleanup:
    zstr_free (&filename);
    zfile_destroy (&logfile);
    if (self->verbose)
        zvector_info (self->clock, "Collect logs %s", zyre_uuid (self->node));

    return messages;
}


static zmsg_t *
s_zlog_send_collect_log (zecho_t *echo, zlog_t *self)
{
    assert (self);
    zmsg_t *collect_msg = zmsg_new ();

    //  Append collect log messages from peers
    const char *logmsg = (const char *) zlistx_first (self->collect_log);
    while (logmsg) {
        zmsg_addstr (collect_msg, logmsg);
        logmsg = (const char *) zlistx_next (self->collect_log);
    }
    zlistx_purge (self->collect_log);

    zlistx_t *messages = s_zlog_read_log (self);
    logmsg = (const char *) zlistx_first (messages);
    while (logmsg) {
        zmsg_addstr (collect_msg, logmsg);
        logmsg = (const char *) zlistx_next (messages);
    }
    zlistx_set_destructor (messages, (zlistx_destructor_fn *) zstr_free);
    zlistx_destroy (&messages);

    return collect_msg;
}


static int
s_zlog_collect_timer (zloop_t *loop, int timer_id, void *arg)
{
    assert (arg);
    zlog_t *self = (zlog_t *) arg;

    if (self->collector)
        zecho_destroy (&self->collector);

    self->collector = zecho_new (self->node);
    zecho_set_clock (self->collector, self->clock);
    zecho_set_collect_handler (self->collector, self);
    zecho_set_collect_process (self->collector, (zecho_process_fn *) s_zlog_process_collect_log);
    zecho_init (self->collector);
    if (self->verbose)
        zvector_info (self->clock, "Start log collection %s\n", zyre_uuid (self->node));

    //  Read and insert leader log
    zlistx_t *messages = s_zlog_read_log (self);
    /*printf ("Lines read %d %d\n", self->linesRead, (int) zlistx_size (messages));*/
    char *logmsg = (char *) zlistx_first (messages);
    while (logmsg) {
        zlistx_insert (self->ordered_log, logmsg, true);
        logmsg = (char *) zlistx_next (messages);
    }
    /*printf ("Lines read %d %d\n", self->linesRead, (int) zlistx_size (self->ordered_log));*/
    zlistx_destroy (&messages);

    return 0;
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

                //  Leader action
                if (zelection_won (self->election))
                    self->leader_timer = zloop_timer (loop, 5000, 0, s_zlog_collect_timer, self);
            }
            //  rc == -1, will be ignored! We just let the election starve.
        }
        else
        if (streq (command, "ZECHO")) {
            if (!self->collector) {
                self->collector = zecho_new (self->node);
                zecho_set_clock (self->collector, self->clock);
                zecho_set_collect_handler (self->collector, self);
                zecho_set_collect_process (self->collector, (zecho_process_fn *) s_zlog_process_collect_log);
                zecho_set_collect_create (self->collector, (zecho_create_fn *) s_zlog_send_collect_log);
            }
            if (zecho_recv (self->collector, event) == 1)
                zecho_destroy (&self->collector);

        }
        else
        if (streq (command, "BAKERY")) {
            char *content = zmsg_popstr (zyre_event_msg (event));
            char *owner = zmsg_popstr (zyre_event_msg (event));
            zvector_info (self->clock, "R: %s - %.5s", content, owner);

            zstr_sendm (self->pipe, content);
            zstr_send (self->pipe, owner);

            zstr_free (&content);
            zstr_free (&owner);
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
    zloop_start (self->loop);   //  Give control to event loop!
    zlog_destroy (&self);
}


//  --------------------------------------------------------------------------
//  Compares the timestamps's of given logMsg a to logMsg b.
//  Returns -1 if a < b, otherwiese 1

int
zlog_compare_log_msg_ts (const char *log_msg_a, const char *log_msg_b)
{
  unsigned long long *ts_a = s_get_timestamp_from_logMsg ((char *)log_msg_a);
  unsigned long long *ts_b = s_get_timestamp_from_logMsg ((char *)log_msg_b);

  return (*ts_a < *ts_b)? -1: 1;
}


//  --------------------------------------------------------------------------
//  Compares the zvector_t's of given logMsg a to logMsg b.
//  Returns -1 if a < b, otherwiese 1

int
zlog_compare_log_msg_vc (const char *log_msg_a, const char *log_msg_b)
{
  zvector_t *clock_a = s_get_clock_from_log_msg ((char *)log_msg_a);
  zvector_t *clock_b = s_get_clock_from_log_msg ((char *)log_msg_b);

  int ret = zvector_compare_to (clock_a, clock_b);
  zvector_destroy (&clock_a);
  zvector_destroy (&clock_b);

  return ret == 0? 1: ret;
}

//  --------------------------------------------------------------------------
//  Reads log of source filepath and orders it with given
//  pointer to compare_function into destination filepath.

void
zlog_order_log (const char *path_src, const char *path_dst, zlistx_comparator_fn *compare_function)
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
  zlistx_set_comparator (ordered_list, (zlistx_comparator_fn *) compare_function);

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
    char *params1[2] = {"inproc://logger1", "GOSSIP MASTER"};
    zactor_t *zlog = zactor_new (zlog_actor, params1);

    char *params2[2] = {"inproc://logger2", "GOSSIP SLAVE"};
    zactor_t *zlog2 = zactor_new (zlog_actor, params2);

    char *params3[2] = {"inproc://logger3", "GOSSIP SLAVE"};
    zactor_t *zlog3 = zactor_new (zlog_actor, params3);

    /*char *params4[3] = {"inproc://logger4", "logger4", "GOSSIP SLAVE"};*/
    /*zactor_t *zlog4 = zactor_new (zlog_actor, params4);*/

    if (verbose) {
        zstr_send (zlog, "VERBOSE");
        zstr_send (zlog2, "VERBOSE");
        zstr_send (zlog3, "VERBOSE");
        /*zstr_send (zlog4, "VERBOSE");*/
    }

    zstr_send (zlog, "START");
    zstr_send (zlog2, "START");
    zstr_send (zlog3, "START");
    /*zstr_send (zlog4, "START");*/

    //  Give time to interconnect and elect
    zclock_sleep (750);

    //  Give time for log collect to happen
    zclock_sleep (12000);

    zstr_send (zlog, "STOP");
    zstr_send (zlog2, "STOP");
    zstr_send (zlog3, "STOP");
    /*zstr_send (zlog4, "STOP");*/

    //  Give time to disconnect
    zclock_sleep (250);

    zactor_destroy (&zlog);
    zactor_destroy (&zlog2);
    zactor_destroy (&zlog3);
    /*zactor_destroy (&zlog4);*/

    /*zlog_order_log ("/var/log/vc.log", "ordered_vc1.log");*/
    //  @end

    printf ("OK\n");
}
