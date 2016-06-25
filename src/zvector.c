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
    assert (item);
    unsigned long value = *(unsigned long *) item;
    int length = 1;

    while (value) {
        value /= 10;
        length++;
    }

    char *result = (char *) zmalloc (length * sizeof (char));
    sprintf (result, "%lu", *(unsigned long *) item);

    return result;
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

    zframe_t *packed_clock = zmsg_pop (msg);
    zhashx_t *sender_clock = zhashx_unpack_own (packed_clock, s_deserialize_clock_value);
    zhashx_set_destructor (sender_clock, s_destroy_clock_value);

    unsigned long *own_clock_value = (unsigned long *) zhashx_lookup (self->clock, self->own_pid);
    (*own_clock_value)++;

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

  //formation: 'VC:$numberOfClocks;$pid1,$val1;...;$pidx,$valx;\0'
  char *result = NULL;
  int string_length = 5;
  unsigned long tmp_val_length = 0;

  int vector_size = zhashx_size (self->clock);

  // calculate needed chars for allocation
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
    //  Simple create/destroy test
    zvector_t *test1_self = zvector_new ("1231");
    assert (test1_self);
    zhashx_t *test1_sender_clock = zhashx_new ();
    zhashx_set_destructor (test1_sender_clock, s_destroy_clock_value);

    zhashx_destroy (&test1_sender_clock);
    zvector_destroy (&test1_self);



    //  Simple event test
    zvector_t *test2_self = zvector_new ("1231");
    assert (test2_self);
    zhashx_t *test2_sender_clock1 = zhashx_new ();
    zhashx_set_destructor (test2_sender_clock1, s_destroy_clock_value);

    unsigned long *test2_value1 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test2_value1 = 5;
    zhashx_insert (test2_sender_clock1, "1232", test2_value1);

    unsigned long *test2_self_own_clock_value = (unsigned long *) zhashx_lookup (test2_self->clock, "1231");
    assert (*test2_self_own_clock_value == 0);
    unsigned long *test2_self_sender_clock_value = (unsigned long *) zhashx_lookup (test2_sender_clock1, "1232");
    assert (*test2_self_sender_clock_value == 5);

    zvector_event (test2_self);
    test2_self_own_clock_value = (unsigned long *) zhashx_lookup (test2_self->clock, "1231");
    assert (*test2_self_own_clock_value == 1);
    test2_self_sender_clock_value = (unsigned long *) zhashx_lookup (test2_sender_clock1, "1232");
    assert (*test2_self_sender_clock_value == 5);

    zhashx_destroy (&test2_sender_clock1);
    zvector_destroy (&test2_self);


    //  Simple recv test
    zvector_t *test3_self_clock = zvector_new ("1231");
    assert (test3_self_clock);
    zhashx_t *test3_sender_clock1 = zhashx_new ();
    zhashx_set_destructor (test3_sender_clock1, s_destroy_clock_value);
    zhashx_t *test3_sender_clock2 = zhashx_new ();
    zhashx_set_destructor (test3_sender_clock2, s_destroy_clock_value);

    // insert some key-value pairs in test3_sender_clock1
    unsigned long *test3_inserted_value1 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test3_inserted_value1 = 5;
    zhashx_insert (test3_sender_clock1, "1231", test3_inserted_value1);
    unsigned long *test3_inserted_value2 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test3_inserted_value2 = 10;
    zhashx_insert (test3_sender_clock1, "1232", test3_inserted_value2);

    // insert some key-value pairs in test3_sender_clock2
    unsigned long *test3_inserted_value3 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test3_inserted_value3 = 20;
    zhashx_insert (test3_sender_clock2, "1231", test3_inserted_value3);
    unsigned long *test3_inserted_value4 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test3_inserted_value4 = 30;
    zhashx_insert (test3_sender_clock2, "1233", test3_inserted_value4);

    // receive sender clock 1 and add key-value pairs to own clock
    zmsg_t *test3_msg1 = zmsg_new ();
    zframe_t *test3_packed_clock1 = zhashx_pack_own (test3_sender_clock1, s_serialize_clock_value);
    zmsg_prepend (test3_msg1, &test3_packed_clock1);
    zvector_recv (test3_self_clock, test3_msg1);
    zmsg_destroy (&test3_msg1);
    unsigned long *test3_found_value1 = (unsigned long *) zhashx_lookup (test3_self_clock->clock, "1231");
    assert (*test3_found_value1 == 5);
    unsigned long *test3_found_value2 = (unsigned long *) zhashx_lookup (test3_self_clock->clock, "1232");
    assert (*test3_found_value2 == 10);

    // receive sender clock 2 and add key-value pairs to own clock
    zmsg_t *test3_msg2 = zmsg_new ();
    zframe_t *test3_packed_clock2 = zhashx_pack_own (test3_sender_clock2, s_serialize_clock_value);
    zmsg_prepend (test3_msg2, &test3_packed_clock2);
    zvector_recv (test3_self_clock, test3_msg2);
    zmsg_destroy (&test3_msg2);
    unsigned long *test3_found_value3 = (unsigned long *) zhashx_lookup (test3_self_clock->clock, "1231");
    assert (*test3_found_value3 == 20);
    test3_found_value2 = (unsigned long *) zhashx_lookup (test3_self_clock->clock, "1232");
    assert (*test3_found_value2 == 10);
    unsigned long *test3_found_value4 = (unsigned long *) zhashx_lookup (test3_self_clock->clock, "1233");
    assert (*test3_found_value4 == 30);

    zhashx_destroy (&test3_sender_clock1);
    zhashx_destroy (&test3_sender_clock2);
    zvector_destroy (&test3_self_clock);

    // Simple send_prepare test
    zvector_t *test4_self = zvector_new ("1231");
    assert (test4_self);

    zvector_event (test4_self);

    zmsg_t *test4_zmsg = zmsg_new ();
    zmsg_pushstr (test4_zmsg, "test");
    zvector_send_prepare (test4_self, test4_zmsg);

    zframe_t *test4_popped_frame = zmsg_pop (test4_zmsg);
    zhashx_t *test4_unpacked_clock = zhashx_unpack_own (test4_popped_frame, s_deserialize_clock_value);
    unsigned long *test4_found_value1 = (unsigned long *) zhashx_lookup (test4_unpacked_clock, "1231");
    assert (*test4_found_value1 == 2);
    char *test4_unpacked_string = zmsg_popstr (test4_zmsg);
    assert (streq (test4_unpacked_string, "test"));

    zframe_destroy (&test4_popped_frame);
    zstr_free (&test4_unpacked_string);
    zhashx_destroy (&test4_unpacked_clock);
    zmsg_destroy (&test4_zmsg);
    zvector_destroy (&test4_self);



    // Simple to_string test
    zvector_t *test5_self = zvector_new ("1000");
    assert (test5_self);

    // inserting some clocks & values
    zvector_event (test5_self);

    unsigned long *test5_inserted_value1 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test5_inserted_value1 = 7;
    zhashx_insert (test5_self->clock, "1222", test5_inserted_value1);
    unsigned long *test5_inserted_value2 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test5_inserted_value2 = 11;
    zhashx_insert (test5_self->clock, "1444", test5_inserted_value2);

    char *test5_string = zvector_to_string (test5_self);
    assert (streq (test5_string, "VC:3;1000,1;1222,7;1444,11;"));

    zstr_free (&test5_string);
    zvector_destroy (&test5_self);


    //  @end
    printf ("OK\n");
}
