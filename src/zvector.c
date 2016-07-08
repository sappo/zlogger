/*  =========================================================================
    zvector - Implements a dynamic vector clock

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of zlogger.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    zvector - Implements a dynamic vector clock
@discuss
@end
*/

#include "zlog_classes.h"

//  Structure of our class

struct _zvector_t {
    char *own_pid;
    zhashx_t *clock;
    zlist_t *space_time_states;
    zlist_t *space_time_events;
};


//  --------------------------------------------------------------------------
//  Local helper functions

static void
s_destroy_clock_value (void **clock_value_p)
{
    assert (clock_value_p);
    if (*clock_value_p) {
        unsigned long *clock_value = (unsigned long *) *clock_value_p;
        free (clock_value);
    }
}


//  --------------------------------------------------------------------------
//  Create a new zvector

zvector_t *
zvector_new (const char *pid)
{
    zvector_t *self = (zvector_t *) zmalloc (sizeof (zvector_t));

    assert (self);
    //  Initialize class properties here
    self->own_pid = strdup (pid);
    self->clock = zhashx_new ();
    self->space_time_states = zlist_new ();
    self->space_time_events = zlist_new ();
    zhashx_set_destructor (self->clock, s_destroy_clock_value);
    unsigned long *clock_val = (unsigned long *) zmalloc (sizeof (unsigned long));
    *clock_val = 0;
    zhashx_insert (self->clock, pid, clock_val);
    zsys_set_logsystem (true);
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zvector

void
zvector_destroy (zvector_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zvector_t *self = *self_p;
        //  Free class properties here
        zstr_free (&self->own_pid);
        zhashx_destroy (&self->clock);
        zlist_autofree (self->space_time_states);
        zlist_destroy (&self->space_time_states);
        zlist_autofree (self->space_time_events);
        zlist_destroy (&self->space_time_events);
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Event the zvector

void
zvector_event (zvector_t *self)
{
    assert (self);
    unsigned long *own_clock_value = (unsigned long *) zhashx_lookup (self->clock, self->own_pid);
    (*own_clock_value)++;

    zlist_append (self->space_time_states, zvector_to_string (self));
}


//  --------------------------------------------------------------------------
//  Eventing own clock & packing vectorclock with given msg

zmsg_t *
zvector_send_prepare (zvector_t *self, zmsg_t *msg)
{
    assert (self);
    assert (msg);

    zvector_event (self);
    char *clock_string = zvector_to_string (self);
    zmsg_pushstr (msg, clock_string);
    zstr_free (&clock_string);
    return msg;
}


//  --------------------------------------------------------------------------
//  Recv the zvector & updates own vectorclock

void
zvector_recv (zvector_t *self, zmsg_t *msg)
{
    assert (self);
    assert (msg);

    char *clock_string = zmsg_popstr (msg);
    zvector_t *sender_vector = zvector_from_string (clock_string);
    zstr_free (&clock_string);

    zhashx_t *sender_clock = sender_vector->clock;

    zvector_event (self);
    char *self_clock_string = zvector_to_string (self);
    clock_string = zvector_to_string (sender_vector);
    zlist_append (self->space_time_events, zsys_sprintf ("\"%s\" -> \"%s\"\n",
                                                         clock_string, self_clock_string));
    zstr_free (&clock_string);
    zstr_free (&self_clock_string);

    zlistx_t *sender_clock_procs = zhashx_keys (sender_clock);
    const char *pid = (const char *) zlistx_first (sender_clock_procs);
    while (pid) {
        if (zhashx_lookup (self->clock, pid)) {
            unsigned long *own_pid_clock_value = (unsigned long *) zhashx_lookup (self->clock, pid);
            unsigned long *sender_pid_clock_value = (unsigned long *) zhashx_lookup (sender_clock, pid);

            if ( (*sender_pid_clock_value) > (*own_pid_clock_value) )
                 (*own_pid_clock_value) = (*sender_pid_clock_value);
        }
        else{
            unsigned long *sender_pid_clock_value = (unsigned long *) zhashx_lookup (sender_clock, pid);
            unsigned long *own_pid_clock_value = (unsigned long *) zmalloc (sizeof (unsigned long));
            (*own_pid_clock_value) = (*sender_pid_clock_value);
            zhashx_insert (self->clock, pid, own_pid_clock_value);
        }

        pid = (const char*) zlistx_next (sender_clock_procs);
    }

    zlistx_destroy (&sender_clock_procs);
    zvector_destroy (&sender_vector);
}


//  --------------------------------------------------------------------------
//  Converts the zvector into string representation

char *
zvector_to_string (zvector_t *self)
{
  assert (self);

  //formation: 'VC:$numberOfClocks;own:$ownPid;$pid1,$val1;...;$pidx,$valx;\0'
  char *result = NULL;
  int string_length = 10;
  unsigned long tmp_val_length = 0;

  int vector_size = zhashx_size (self->clock);

  // calculate needed chars for allocation
  string_length += strlen(self->own_pid);

  // digits needed for numberOfClocks
  tmp_val_length = vector_size;
  while (tmp_val_length) {
      tmp_val_length /= 10;
      string_length += 1;
  }

  // chars needed for seperators
  string_length += (vector_size * 2);
  unsigned long *value = (unsigned long *) zhashx_first (self->clock);
  const char *pid = (const char *) zhashx_cursor (self->clock);
  tmp_val_length = *value;

  while (value) {
    string_length += strlen (pid);

    // calc digits of clock_value
    while (tmp_val_length) {
        tmp_val_length /= 10;
        string_length += 1;
    }

    value = (unsigned long *) zhashx_next (self->clock);
    pid = (const char*) zhashx_cursor (self->clock);

    if (value)
      tmp_val_length = *value;
  }

  result = (char *) zmalloc (string_length * sizeof (char));

  // fill up string
  value = (unsigned long *) zhashx_first (self->clock);
  pid = (const char *) zhashx_cursor (self->clock);
  char tmp_string[11]; // max digits of an unsigned long value
  tmp_string[10] = '\0';

  sprintf(result, "VC:%d;", vector_size);
  strcat(result, "own:");
  strcat(result, self->own_pid);
  strcat(result, ";");
  while (value) {
    strcat (result, pid);
    strcat (result, ",");
    sprintf(tmp_string, "%lu", *value);
    strcat (result, tmp_string);
    strcat (result, ";");

    value = (unsigned long *) zhashx_next (self->clock);
    pid = (const char *) zhashx_cursor (self->clock);
  }

  return result;
}


//  --------------------------------------------------------------------------
//  Creates a zvector from a given string representation

zvector_t *
zvector_from_string (char *clock_string)
{
    assert (clock_string);
    assert (clock_string[0] == 'V');
    assert (clock_string[1] == 'C');

    //formation: 'VC:$numberOfClocks;own:$ownPid;$pid1,$val1;...;$pidx,$valx;\0'
    unsigned long vc_count = 0;
    zvector_t *ret = NULL;

    int state = 0;
    char *needle, *pid = NULL, *beginWord = NULL;
    size_t len = strlen (clock_string);
    needle = (char *) clock_string;
    char *needle_stop = needle + len;
    beginWord = needle;

    while (needle < needle_stop + 1) {
        if (*needle == ':') {
            char *word = strndup (beginWord, needle - beginWord);
            beginWord = needle + 1;
            if (streq (word, "VC"))
                state = 1;
            else
            if (streq (word, "own"))
                state = 2;
            else
                assert (false);

            zstr_free (&word);
        }
        else
        if (*needle == ',') {
            pid = strndup (beginWord, needle - beginWord);
            beginWord = needle + 1;
            state = 3;
        }
        else
        if (*needle == ';') {
            assert (state != 0);
            char *word = strndup (beginWord, needle - beginWord);
            if (state == 1)
                vc_count = strtoul (word, NULL, 10);
            else
            if (state == 2) {
                ret = zvector_new (word);
                zhashx_purge (ret->clock);
            }
            else
            if (state == 3) {
                unsigned long *clock_value = (unsigned long *) zmalloc (sizeof (unsigned long));
                *clock_value = strtoul(word, NULL, 10);
                zhashx_insert (ret->clock, pid, clock_value);
                zstr_free (&pid);
            }
            beginWord = needle + 1;
            state = 0;
            zstr_free (&word);
        }
        needle++;
    }
    assert (vc_count == zhashx_size (ret->clock));

    return ret;
}


//  --------------------------------------------------------------------------
//  Compares two zvector. Returns -1 at happened before self, 0 at parallel,
//  1 at happened after.

int
zvector_compare_to (zvector_t *self, zvector_t *other)
{
    assert (self);
    assert (other);

    //  self => other
    unsigned long *self_val = (unsigned long *) zhashx_lookup (self->clock, self->own_pid);
    unsigned long *other_val = (unsigned long *) zhashx_lookup (other->clock, self->own_pid);
    if (self_val && other_val && *self_val <= *other_val)
        return -1;

    //  other => self
    self_val = (unsigned long *) zhashx_lookup (self->clock, other->own_pid);
    other_val = (unsigned long *) zhashx_lookup (other->clock, other->own_pid);
    if (self_val && other_val && *other_val <= *self_val)
        return 1;

    //  self || other
    return 0;
}


//  --------------------------------------------------------------------------
//  Log informational message - low priority. Prepends the current VC.

void
zvector_info (zvector_t *self, char *format, ...)
{
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *logmsg = zsys_vprintf (format, argptr);
    va_end (argptr);
    char *clockstr = zvector_to_string (self);
    zsys_info ("/%s/ %s", clockstr, logmsg);
    zstr_free (&logmsg);
    zstr_free (&clockstr);
    zvector_event (self);
}

void
zvector_dump_time_space (zvector_t *self)
{
    assert (self);
    FILE *file_dst = fopen(self->own_pid, "w");
    assert (file_dst);
    const char newLineSymbol = '\n';
    const char *line;

    char *subgraph_string = zsys_sprintf ("subgraph c_%s {\n" \
                                          "  label = \"P#%s\"\n",
                                          self->own_pid,
                                          self->own_pid);
    fwrite (subgraph_string, 1, strlen (subgraph_string), file_dst);
    zstr_free (&subgraph_string);

    line = (const char *) zlist_first (self->space_time_states);
    while (line != NULL) {
        fwrite ("\"", 1, 1, file_dst);
        fwrite (line, 1, strlen (line), file_dst);
        fwrite ("\"", 1, 1, file_dst);
        line = (const char *) zlist_next (self->space_time_states);
        if (line)
            fwrite (" -> ", 1, 4, file_dst);
    }

    fwrite (";\n}\n", 1, 4, file_dst);

    line = (const char *) zlist_first (self->space_time_events);
    while (line != NULL) {
        fwrite (line, 1, strlen (line), file_dst);
        fwrite (";", 1, 1, file_dst);
        fwrite (&newLineSymbol, 1, 1, file_dst);
        line = (const char *) zlist_next (self->space_time_events);
    }
    fclose (file_dst);
}


//  --------------------------------------------------------------------------
//  Duplicates the given zvector, returns a freshly allocated dulpicate.

zvector_t *
zvector_dup (zvector_t *self)
{
    assert (self);

    zvector_t *dup = zvector_new (self->own_pid);
    zhashx_purge (dup->clock);

    unsigned long *value = (unsigned long *) zhashx_first (self->clock);
    const char *pid = (const char *) zhashx_cursor (self->clock);
    while (value) {
      zhashx_insert (dup->clock, pid, value);
      value = (unsigned long *) zhashx_next (self->clock);
      pid = (const char *) zhashx_cursor (self->clock);
    }

    return dup;
}


//  --------------------------------------------------------------------------
//  Prints the zvector for debug purposes

void
zvector_print (zvector_t *self)
{
  assert (self);

  int vector_size = zhashx_size (self->clock);
  unsigned long *value = (unsigned long *) zhashx_first (self->clock);
  const char *pid = (const char *) zhashx_cursor (self->clock);

  printf ("\n\tpid\tvalue\tsize: %d\n", vector_size);

  while (value) {
    printf ("\t%s\t%lu\n", pid, *value);

    value = (unsigned long *) zhashx_next (self->clock);
    pid = (const char *) zhashx_cursor (self->clock);
  }

}


//  --------------------------------------------------------------------------
//  Self test of this class

void
zvector_test (bool verbose)
{
    printf (" * zvector: ");

    //  @selftest
    //  TEST: create/destroy zvector_t
    zvector_t *test1_self = zvector_new ("1000");
    assert (test1_self);
    zvector_destroy (&test1_self);

    //  TEST: for converting a zvector to stringrepresentation and from
    //        string represenstation to zvector.
    zvector_t *test2_self = zvector_new ("1000");
    assert (test2_self);

    //  Inserting some clocks & values
    zvector_event (test2_self);
    unsigned long *test2_inserted_value1 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test2_inserted_value1 = 7;
    zhashx_insert (test2_self->clock, "1001", test2_inserted_value1);
    unsigned long *test2_inserted_value2 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test2_inserted_value2 = 11;
    zhashx_insert (test2_self->clock, "1002", test2_inserted_value2);

    char *test2_string = zvector_to_string (test2_self);
    assert (streq (test2_string, "VC:3;own:1000;1001,7;1000,1;1002,11;"));

    zvector_t *test2_generated = zvector_from_string (test2_string);
    assert ( *(unsigned long *) zhashx_lookup (test2_generated->clock, "1000") == 1 );
    assert ( *(unsigned long *) zhashx_lookup (test2_generated->clock, "1001") == 7 );
    assert ( *(unsigned long *) zhashx_lookup (test2_generated->clock, "1002") == 11 );

    zstr_free (&test2_string);
    zvector_destroy (&test2_self);
    zvector_destroy (&test2_generated);

    //  TEST: events
    zvector_t *test3_self = zvector_new ("1000");
    assert (test3_self);

    unsigned long *test3_value1 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test3_value1 = 5;
    zhashx_insert (test3_self->clock, "1001", test3_value1);
    assert ( *(unsigned long *) zhashx_lookup (test3_self->clock, "1000") == 0 );

    zvector_event (test3_self);
    assert ( *(unsigned long *) zhashx_lookup (test3_self->clock, "1000") == 1 );
    assert ( *(unsigned long *) zhashx_lookup (test3_self->clock, "1001") == 5 );
    zvector_destroy (&test3_self);

    //  TEST: recv test
    zvector_t *test4_self_clock = zvector_new ("1000");
    char *test4_sender_clock1_stringRep = zsys_sprintf ("%s", "VC:2;own:1001;1000,5;1001,10;");
    char *test4_sender_clock2_stringRep = zsys_sprintf ("%s", "VC:2;own:1002;1000,20;1002,30;");
    assert (test4_self_clock);

    //  Receive sender clock 1 and add key-value pairs to own clock
    zmsg_t *test4_msg1 = zmsg_new ();
    zmsg_pushstr (test4_msg1, test4_sender_clock1_stringRep);
    zvector_recv (test4_self_clock, test4_msg1);
    zmsg_destroy (&test4_msg1);
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1000") == 5 );
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1001") == 10 );

    //  Receive sender clock 2 and add key-value pairs to own clock
    test4_msg1 = zmsg_new ();
    zmsg_pushstr (test4_msg1, test4_sender_clock2_stringRep);
    zvector_recv (test4_self_clock, test4_msg1);
    zmsg_destroy (&test4_msg1);
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1000") == 20 );
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1001") == 10 );
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1002") == 30 );

    zstr_free (&test4_sender_clock1_stringRep);
    zstr_free (&test4_sender_clock2_stringRep);
    zvector_destroy (&test4_self_clock);

    // TEST: send prepare
    zvector_t *test5_self = zvector_new ("1000");
    assert (test5_self);
    zvector_event (test5_self);
    zmsg_t *test5_zmsg = zmsg_new ();
    zmsg_pushstr (test5_zmsg, "test");

    zvector_send_prepare (test5_self, test5_zmsg);
    char *test5_clock_string = zmsg_popstr (test5_zmsg);
    zvector_t *test5_unpacked_clock = zvector_from_string (test5_clock_string);
    char *test5_unpacked_string = zmsg_popstr (test5_zmsg);
    assert (streq (test5_unpacked_string, "test"));
    assert ( *(unsigned long *) zhashx_lookup (test5_unpacked_clock->clock, "1000") == 2 );

    zstr_free (&test5_clock_string);
    zstr_free (&test5_unpacked_string);
    zmsg_destroy (&test5_zmsg);
    zvector_destroy (&test5_self);
    zvector_destroy (&test5_unpacked_clock);

    // TEST: compare
    char *test6_self_stringrep = zsys_sprintf ("%s", "VC:3;own:p2;p1,2;p2,2;p3,2;");
    char *test6_before_stringrep1 = zsys_sprintf ("%s", "VC:2;own:p3;p1,1;p3,2;");
    char *test6_before_stringrep2 = zsys_sprintf ("%s", "VC:2;own:p2;p1,2;p2,1;");
    char *test6_before_stringrep3 = zsys_sprintf ("%s", "VC:1;own:p1;p1,1;");
    char *test6_parallel_stringrep1 = zsys_sprintf ("%s", "VC:1;own:p1;p1,3;");
    char *test6_parallel_stringrep2 = zsys_sprintf ("%s", "VC:2;own:p3;p1,1;p3,3;");
    char *test6_parallel_stringrep3 = zsys_sprintf ("%s", "VC:2;own:p1;p1,4;p3,3;");
    char *test6_after_stringrep1 = zsys_sprintf ("%s", "VC:3;own:p2;p1,2;p2,3;p3,2;");
    char *test6_after_stringrep2 = zsys_sprintf ("%s", "VC:3;own:p3;p1,2;p2,3;p3,4;");
    zvector_t *test6_self = zvector_from_string (test6_self_stringrep);
    zvector_t *test6_before1 = zvector_from_string (test6_before_stringrep1);
    zvector_t *test6_before2 = zvector_from_string (test6_before_stringrep2);
    zvector_t *test6_before3 = zvector_from_string (test6_before_stringrep3);
    zvector_t *test6_parallel1 = zvector_from_string (test6_parallel_stringrep1);
    zvector_t *test6_parallel2 = zvector_from_string (test6_parallel_stringrep2);
    zvector_t *test6_parallel3 = zvector_from_string (test6_parallel_stringrep3);
    zvector_t *test6_after1 = zvector_from_string (test6_after_stringrep1);
    zvector_t *test6_after2 = zvector_from_string (test6_after_stringrep2);

    assert (zvector_compare_to (test6_self, test6_before1) == 1);
    assert (zvector_compare_to (test6_self, test6_before2) == 1);
    assert (zvector_compare_to (test6_self, test6_before3) == 1);
    assert (zvector_compare_to (test6_self, test6_parallel1) == 0);
    assert (zvector_compare_to (test6_self, test6_parallel2) == 0);
    assert (zvector_compare_to (test6_self, test6_parallel3) == 0);
    assert (zvector_compare_to (test6_self, test6_after1) == -1);
    assert (zvector_compare_to (test6_self, test6_after2) == -1);

    zstr_free (&test6_self_stringrep);
    zstr_free (&test6_before_stringrep1);
    zstr_free (&test6_before_stringrep2);
    zstr_free (&test6_before_stringrep3);
    zstr_free (&test6_parallel_stringrep1);
    zstr_free (&test6_parallel_stringrep2);
    zstr_free (&test6_parallel_stringrep3);
    zstr_free (&test6_after_stringrep1);
    zstr_free (&test6_after_stringrep2);

    zvector_destroy (&test6_self);
    zvector_destroy (&test6_before1);
    zvector_destroy (&test6_before2);
    zvector_destroy (&test6_before3);
    zvector_destroy (&test6_parallel1);
    zvector_destroy (&test6_parallel2);
    zvector_destroy (&test6_parallel3);
    zvector_destroy (&test6_after1);
    zvector_destroy (&test6_after2);
    //  @end
    printf ("OK\n");
}
