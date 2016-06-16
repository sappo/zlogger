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
    char* own_pid;
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

//  --------------------------------------------------------------------------
//  Create a new zvector

zvector_t *
zvector_new (const char* pid)
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
    unsigned long *own_clock_value = (unsigned long*) zhashx_lookup (self->clock, self->own_pid);
    (*own_clock_value)++;
}


//  --------------------------------------------------------------------------
//  Send the zvector


zmsg_t *
zvector_send_prepare (zvector_t *self, zmsg_t *msg)
{
    zvector_event (self);
    zframe_t *packed_clock = zhashx_pack (self->clock);
    zmsg_prepend (msg, &packed_clock);
    return msg;
}


//  --------------------------------------------------------------------------
//  Recv the zvector


void
zvector_recv (zvector_t *self, zmsg_t *msg)
{
    assert (self);

    zframe_t *packed_clock = zmsg_pop (msg);
    zhashx_t *sender_clock = zhashx_unpack (packed_clock);
    zhashx_set_destructor (sender_clock, s_destroy_clock_value);

    unsigned long *own_clock_value = (unsigned long*) zhashx_lookup (self->clock, self->own_pid);
    (*own_clock_value)++;

    zlistx_t *sender_clock_procs = zhashx_keys (sender_clock);
    const char* pid = (const char*) zlistx_first (sender_clock_procs);
    while (pid) {
        if (zhashx_lookup (self->clock, pid)) {
            unsigned long *own_pid_clock_value = (unsigned long*) zhashx_lookup (self->clock, pid);
            unsigned long *sender_pid_clock_value = (unsigned long*) zhashx_lookup (sender_clock, pid);

            if ( (*sender_pid_clock_value) > (*own_pid_clock_value) )
                 *own_pid_clock_value = *sender_pid_clock_value;
        }
        else{
            unsigned long *sender_pid_clock_value = (unsigned long*) zhashx_lookup (sender_clock, pid);
            unsigned long *own_pid_clock_value = (unsigned long *) zmalloc (sizeof (unsigned long));
            *own_pid_clock_value = *sender_pid_clock_value;
            zhashx_insert (self->clock, pid, own_pid_clock_value);
        }

        pid = (const char*) zlistx_next (sender_clock_procs);
    }

    zframe_destroy (&packed_clock);
    zlistx_destroy (&sender_clock_procs);
    zhashx_destroy (&sender_clock);
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

    unsigned long *test2_self_own_clock_value = (unsigned long*) zhashx_lookup (test2_self->clock, "1231");
    assert (*test2_self_own_clock_value == 0);
    unsigned long *test2_self_sender_clock_value = (unsigned long*) zhashx_lookup (test2_sender_clock1, "1232");
    assert (*test2_self_sender_clock_value == *test2_value1);

    zvector_event (test2_self);
    test2_self_own_clock_value = (unsigned long*) zhashx_lookup (test2_self->clock, "1231");
    assert (*test2_self_own_clock_value == 1);
    test2_self_sender_clock_value = (unsigned long*) zhashx_lookup (test2_sender_clock1, "1232");
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
    zframe_t *test3_packed_clock1 = zhashx_pack (test3_sender_clock1);
    zmsg_prepend (test3_msg1, &test3_packed_clock1);
    zvector_recv (test3_self_clock, test3_msg1);
    zmsg_destroy (&test3_msg1);
    unsigned long *test3_found_value1 = (unsigned long*) zhashx_lookup (test3_self_clock->clock, "1231");
    assert (*test3_found_value1 == 5);
    unsigned long *test3_found_value2 = (unsigned long*) zhashx_lookup (test3_self_clock->clock, "1232");
    assert (*test3_found_value2 == 10);

    // receive sender clock 2 and add key-value pairs to own clock
    zmsg_t *test3_msg2 = zmsg_new ();
    zframe_t *test3_packed_clock2 = zhashx_pack (test3_sender_clock2);
    zmsg_prepend (test3_msg2, &test3_packed_clock2);
    zvector_recv (test3_self_clock, test3_msg2);
    zmsg_destroy (&test3_msg2);
    unsigned long *test3_found_value3 = (unsigned long*) zhashx_lookup (test3_self_clock->clock, "1231");
    assert (*test3_found_value3 == 20);
    test3_found_value2 = (unsigned long*) zhashx_lookup (test3_self_clock->clock, "1232");
    assert (*test3_found_value2 == 10);
    unsigned long *test3_found_value4 = (unsigned long*) zhashx_lookup (test3_self_clock->clock, "1233");
    assert (*test3_found_value4 == 30);

    zhashx_destroy (&test3_sender_clock1);
    zhashx_destroy (&test3_sender_clock2);
    zvector_destroy (&test3_self_clock);



    // Simple send prepare test
    zvector_t *test4_self = zvector_new ("1231");
    assert (test4_self);

    zvector_event (test4_self);

    zmsg_t *test4_zmsg = zmsg_new ();
    zmsg_pushstr (test4_zmsg, "test");
    zvector_send_prepare (test4_self, test4_zmsg);

    zmsg_destroy (&test4_zmsg);
    zvector_destroy (&test4_self);



    //  @end
    printf ("OK\n");
}
