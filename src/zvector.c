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
};


//  --------------------------------------------------------------------------
//  Local helper

static void
s_destroy_clock_value (void **clock_value_p)
{
    assert (clock_value_p);
    if (*clock_value_p) {
        unsigned long *clock_value = (unsigned long *) *clock_value_p;
        free (clock_value);
    }
}


//  Serializes an unsigned long into a string

static char *
s_serialize_clock_value (const void *item)
{
    return zsys_sprintf ("%lu", *(unsigned long *) item);
}


//  Deserializes a string representation of clock value into an unsigned long

static void *
s_deserialize_clock_value (const char *item)
{
    assert (item);
    unsigned long *result = (unsigned long *) zmalloc (sizeof (unsigned long));
    sscanf (item, "%lu", result);

    return result;
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
}


//  --------------------------------------------------------------------------
//  Eventing own clock & packing vectorclock with given msg

zmsg_t *
zvector_send_prepare (zvector_t *self, zmsg_t *msg)
{
    assert (self);
    assert (msg);

    zvector_event (self);
    zframe_t *packed_clock = zhashx_pack_own (self->clock, s_serialize_clock_value);
    zmsg_prepend (msg, &packed_clock);
    return msg;
}


//  --------------------------------------------------------------------------
//  Recv the zvector & updates own vectorclock

void
zvector_recv (zvector_t *self, zmsg_t *msg)
{
    assert (self);
    assert (msg);

    zframe_t *packed_clock = zmsg_pop (msg);
    zhashx_t *sender_clock = zhashx_unpack_own (packed_clock, s_deserialize_clock_value);
    zhashx_set_destructor (sender_clock, s_destroy_clock_value);

    zvector_event (self);

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

    zframe_destroy (&packed_clock);
    zlistx_destroy (&sender_clock_procs);
    zhashx_destroy (&sender_clock);
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


  char *pid_ptr = NULL;
  char *val_ptr = NULL;
  char *own_pid_ptr = NULL;

  //formation: 'VC:$numberOfClocks;own:$ownPid;$pid1,$val1;...;$pidx,$valx;\0'
  val_ptr = strtok(clock_string, ":");
  val_ptr = strtok(NULL, ";");
  int numberOfEntries = strtoul(val_ptr, NULL, 10);

  own_pid_ptr = strtok(NULL, ":");
  own_pid_ptr = strtok(NULL, ";");
  zvector_t *ret = zvector_new (own_pid_ptr);
  zhashx_purge (ret->clock);

  for (int i=0; i<numberOfEntries; i++) {
    pid_ptr = strtok(NULL, ",");
    val_ptr = strtok(NULL, ";");
    unsigned long *clock_value = (unsigned long *) zmalloc (sizeof (unsigned long));
    *clock_value = strtoul(val_ptr, NULL, 10);

    zhashx_insert (ret->clock, pid_ptr, clock_value);
  }

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



    //  Simple create/destroy test of zvector_t
    zvector_t *test1_self = zvector_new ("1000");
    assert (test1_self);
    zvector_destroy (&test1_self);



    // Simple test for converting a zvector to stringrepresentation
    // and from stringrepresentation to a zvector
    zvector_t *test2_self = zvector_new ("1000");
    assert (test2_self);

    // inserting some clocks & values
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



    //  Simple event test
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



    //  Simple recv test
    zvector_t *test4_self_clock = zvector_new ("1000");
    char *test4_sender_clock1_stringRep = zsys_sprintf ("%s", "VC:2;own:1001;1000,5;1001,10;");
    char *test4_sender_clock2_stringRep = zsys_sprintf ("%s", "VC:2;own:1002;1000,20;1002,30;");
    zvector_t *test4_sender_clock1 = zvector_from_string (test4_sender_clock1_stringRep);
    zvector_t *test4_sender_clock2 = zvector_from_string (test4_sender_clock2_stringRep);
    assert (test4_self_clock);
    assert (test4_sender_clock1);
    assert (test4_sender_clock2);

    // receive sender clock 1 and add key-value pairs to own clock
    zmsg_t *test4_msg1 = zmsg_new ();
    zframe_t *test4_packed_clock1 = zhashx_pack_own (test4_sender_clock1->clock, s_serialize_clock_value);
    zmsg_prepend (test4_msg1, &test4_packed_clock1);
    zvector_recv (test4_self_clock, test4_msg1);
    zmsg_destroy (&test4_msg1);
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1000") == 5 );
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1001") == 10 );

    // receive sender clock 2 and add key-value pairs to own clock
    test4_msg1 = zmsg_new ();
    zframe_t *test4_packed_clock2 = zhashx_pack_own (test4_sender_clock2->clock, s_serialize_clock_value);
    zmsg_prepend (test4_msg1, &test4_packed_clock2);
    zvector_recv (test4_self_clock, test4_msg1);
    zmsg_destroy (&test4_msg1);
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1000") == 20 );
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1001") == 10 );
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1002") == 30 );

    zstr_free (&test4_sender_clock1_stringRep);
    zstr_free (&test4_sender_clock2_stringRep);
    zvector_destroy (&test4_self_clock);
    zvector_destroy (&test4_sender_clock1);
    zvector_destroy (&test4_sender_clock2);



    // Simple send_prepare test
    zvector_t *test5_self = zvector_new ("1000");
    assert (test5_self);
    zvector_event (test5_self);
    zmsg_t *test5_zmsg = zmsg_new ();
    zmsg_pushstr (test5_zmsg, "test");

    zvector_send_prepare (test5_self, test5_zmsg);
    zframe_t *test5_popped_frame = zmsg_pop (test5_zmsg);
    zhashx_t *test5_unpacked_clock = zhashx_unpack_own (test5_popped_frame, s_deserialize_clock_value);
    char *test5_unpacked_string = zmsg_popstr (test5_zmsg);
    assert (streq (test5_unpacked_string, "test"));
    assert ( *(unsigned long *) zhashx_lookup (test5_unpacked_clock, "1000") == 2 );

    zframe_destroy (&test5_popped_frame);
    zstr_free (&test5_unpacked_string);
    zhashx_destroy (&test5_unpacked_clock);
    zmsg_destroy (&test5_zmsg);
    zvector_destroy (&test5_self);


    // Simple compare test
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
